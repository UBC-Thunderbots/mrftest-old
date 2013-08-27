#include "log.h"
#include "io.h"
#include "sdcard.h"
#include "syscalls.h"
#include <stdio.h>
#include <string.h>

#define SECTOR_SIZE 512U
#define BUFFER_SECTORS 4U
#define RECORDS_PER_SECTOR (SECTOR_SIZE / LOG_RECORD_SIZE)

typedef int LOG_RECORD_T_IS_TOO_FAT[sizeof(log_record_t) == LOG_RECORD_SIZE ? 1 : -1];
typedef int LOG_RECORD_T_IS_NOT_DIVISIBLE_BY_SECTOR_SIZE[SECTOR_SIZE % LOG_RECORD_SIZE == 0 ? 1 : -1];

static log_record_t buffers[BUFFER_SECTORS][RECORDS_PER_SECTOR];
static log_state_t state = LOG_STATE_UNINITIALIZED;
static uint8_t next_write_buffer, next_alloc_buffer, next_alloc_record;
static uint16_t epoch;
static uint32_t next_write_sector;
static uint32_t last_time_lsb = 0, last_time_msb = 0;

static char CARD_FULL_MESSAGE[] = "LOG: Card full\n";

// next_write_buffer is the index of the next buffer to write to the card.
// If a write operation is ongoing, the write operation is writing buffer next_write_buffer-1.
//
// next_alloc_buffer is usually the index of the buffer in which the next record is allocated.
// However, if next_alloc_record is RECORDS_PER_SECTOR, then next_alloc_buffer the next allocation will be in next_alloc_buffer+1.
//
// In all cases, records [next_write_buffer][0] through [next_alloc_buffer][next_alloc_record-1] are used.
// If a write is ongoing, then [next_write_buffer-1][0…RECORDS_PER_SECTOR-1] are also used.
//
// If next_alloc_record = RECORDS_PER_SECTOR then [next_write_buffer][0…RECORDS_PER_SECTOR-1] are used.
// If next_alloc_record = 0 then [next_write_buffer][0…RECORDS_PER_SECTOR-1] are free or being written.

log_state_t log_state(void) {
	return state;
}

bool log_init(void) {
	// Sanity check.
	if (state != LOG_STATE_UNINITIALIZED) {
		return false;
	}

	// Binary search for the first empty sector.
	{
		uint32_t low = 0, high = sd_sector_count();
		while (low != high && sd_status() == SD_STATUS_OK) {
			uint32_t probe = (low + high) / 2;
			if (!sd_read(probe, &buffers[0])) {
				state = LOG_STATE_SD_ERROR;
				return false;
			}
			if (buffers[0][0].magic == LOG_MAGIC_TICK) {
				low = probe + 1;
			} else {
				high = probe;
			}
		}
		next_write_sector = low;
	}

	// Check if the card is completely full.
	if (next_write_sector == sd_sector_count()) {
		syscall_debug_puts(CARD_FULL_MESSAGE);
		state = LOG_STATE_CARD_FULL;
		return false;
	}

	// Compute the epoch: previous sector’s epoch + 1 (if first empty sector is not first sector), or 1 (if first empty sector is first sector).
	if (next_write_sector > 0) {
		if (!sd_read(next_write_sector - 1, &buffers[0])) {
			state = LOG_STATE_SD_ERROR;
			return false;
		}
		epoch = buffers[0][0].epoch + 1;
	} else {
		epoch = 1;
	}

	// Start the write operation.
	if (!sd_write_multi_start(next_write_sector)) {
		state = LOG_STATE_SD_ERROR;
		return false;
	}

	{
		char buffer[48];
		siprintf(buffer, "LOG: Start epoch %" PRIu16 " at sector %" PRIu32 "\n", epoch, next_write_sector);
		syscall_debug_puts(buffer);
	}
	next_write_buffer = 0;
	next_alloc_buffer = 0;
	next_alloc_record = 0;
	state = LOG_STATE_OK;
	return true;
}

void log_deinit(void) {
	// Sanity check.
	if (state != LOG_STATE_OK && state != LOG_STATE_CARD_FULL) {
		return;
	}

	// Wait for any running transfer to finish.
	while (sd_write_multi_busy());
	if (sd_status() != SD_STATUS_OK) {
		return;
	}

	// Flush out any full buffers.
	while (next_write_buffer != next_alloc_buffer) {
		if (!sd_write_multi_sector(&buffers[next_write_buffer])) {
			state = LOG_STATE_SD_ERROR;
			return;
		}
		next_write_buffer = (next_write_buffer + 1) % BUFFER_SECTORS;
		while (sd_write_multi_busy());
		if (sd_status() != SD_STATUS_OK) {
			return;
		}
	}

	// If some records have been committed to the current sector, we must wipe the rest of the records and write out the sector.
	if (next_alloc_record) {
		while (next_alloc_record != RECORDS_PER_SECTOR) {
			memset(&buffers[next_alloc_buffer][next_alloc_record], 0, sizeof(log_record_t));
			++next_alloc_record;
		}
		if (!sd_write_multi_sector(&buffers[next_alloc_buffer])) {
			state = LOG_STATE_SD_ERROR;
			return;
		}
		while (sd_write_multi_busy());
		if (sd_status() != SD_STATUS_OK) {
			return;
		}
	}

	// Stop the SD operation, if any is running.
	if (sd_write_multi_active()) {
		if (!sd_write_multi_end()) {
			state = LOG_STATE_SD_ERROR;
			return;
		}
	}

	state = LOG_STATE_UNINITIALIZED;
}

static void try_start_write(void) {
	if (!sd_write_multi_busy()) {
		if (next_write_buffer != next_alloc_buffer || next_alloc_record == RECORDS_PER_SECTOR) {
			sd_write_multi_sector(&buffers[next_write_buffer]);
			next_write_buffer = (next_write_buffer + 1) % BUFFER_SECTORS;
		}
	}
}

static log_record_t *log_alloc_impl(void) {
	// Sanity check.
	if (state != LOG_STATE_OK && state != LOG_STATE_OVERFLOW) {
		return 0;
	}

	// Check for full card.
	if (next_write_sector == sd_sector_count()) {
		syscall_debug_puts(CARD_FULL_MESSAGE);
		state = LOG_STATE_CARD_FULL;
		return 0;
	}

	// See if we can fix up next_alloc_record == RECORDS_PER_SECTOR by stepping to the next buffer.
	// If a write is active, then next_write_buffer-1 is being written.
	// So, we cannot step next_alloc_buffer to next_write_buffer-1 as then we would allocate from the sector being written.
	// This is the case when, currently, next_alloc_buffer+1 == next_write_buffer-1, or equivalently, next_alloc_buffer+2 == next_write_buffer.
	// If a write is not active, it is always safe to step this up.
	// It is not possible that a write is not active but the buffers are full, because then we would have started a write just above.
	if (next_alloc_record == RECORDS_PER_SECTOR) {
		if (!sd_write_multi_busy() || ((next_alloc_buffer + 2) % BUFFER_SECTORS) != next_write_buffer) {
			next_alloc_buffer = (next_alloc_buffer + 1) % BUFFER_SECTORS;
			next_alloc_record = 0;
		}
	}

	// Try starting a write.
	try_start_write();

	// Try to allocate a record.
	// The buffer is full if next_alloc_record == RECORDS_PER_SECTOR.
	// This cannot be true in any other case because, if it were, it would have been fixed above.
	if (next_alloc_record == RECORDS_PER_SECTOR) {
		state = LOG_STATE_OVERFLOW;
		return 0;
	} else {
		state = LOG_STATE_OK;
		return &buffers[next_alloc_buffer][next_alloc_record];
	}
}

log_record_t *log_alloc(void) {
	log_record_t *rec = log_alloc_impl();

	if (rec) {
		uint32_t now = IO_SYSCTL.tsc;
		if (now < last_time_lsb) {
			++last_time_msb;
		}
		last_time_lsb = now;

		rec->epoch = epoch;
		rec->time_lsb = last_time_lsb;
		rec->time_msb = last_time_msb;
	}

	return rec;
}

void log_commit(void) {
	// Sanity check.
	if (state != LOG_STATE_OK) {
		return;
	}

	// Commit the record and try starting a write operation.
	++next_alloc_record;
	try_start_write();
}

