#include "log.h"
#include "sdcard.h"
#include "tsc.h"
#include <stdio.h>
#include <string.h>

#define SECTOR_SIZE 512U
#define RECORDS_PER_SECTOR (SECTOR_SIZE / LOG_RECORD_SIZE)

typedef int LOG_RECORD_T_IS_TOO_FAT[sizeof(log_record_t) == LOG_RECORD_SIZE ? 1 : -1];
typedef int LOG_RECORD_T_IS_NOT_DIVISIBLE_BY_SECTOR_SIZE[SECTOR_SIZE % LOG_RECORD_SIZE == 0 ? 1 : -1];

static log_record_t buffers[2][RECORDS_PER_SECTOR];
static log_state_t state = LOG_STATE_UNINITIALIZED;
static uint8_t next_alloc_buffer, next_alloc_record;
static uint16_t epoch;
static uint32_t next_write_sector;
static uint32_t last_time_lsb = 0, last_time_msb = 0;

// State machine:
//
// (1) If next_alloc_record < LOG_RECORDS_PER_SECTOR, then:
//   (1a) If no DMA operation is in progress, then:
//     buffers[next_alloc_buffer][0 … next_alloc_record-1] are used
//     buffers[next_alloc_buffer][next_alloc_record … RECORDS_PER_SECTOR-1] are free and will be used next
//     buffers[!next_alloc_buffer] are free and will be used later
//     when buffers[next_alloc_buffer][RECORDS_PER_SECTOR-1] is committed:
//       buffers[next_alloc_buffer] is sent to DMA,
//       next_alloc_buffer is set to !next_alloc_buffer, and
//       next_alloc_record is set to zero, yielding state (1b)
//
//   (1b) If a DMA operation is in progress, then:
//     buffers[next_alloc_buffer][0 … next_alloc_record-1] are used
//     buffers[next_alloc_buffer][next_alloc_record … RECORDS_PER_SECTOR-1] are free and will be used next
//     buffers[!next_alloc_buffer] are being written to card
//     when the DMA finishes:
//       buffers[!next_alloc_buffer] are no longer needed and, with no further action, state (1a) applies
//     when buffers[next_alloc_buffer][RECORDS_PER_SECTOR-1] is committed:
//       next_alloc_record is set to RECORDS_PER_SECTOR, yielding state (2b)
//
// (2) If next_alloc_record == RECORDS_PER_SECTOR, then:
//   (2a) If no DMA operation is in progress, then:
//     buffers[next_alloc_buffer] are used
//     buffers[!next_alloc_buffer] are free and will be used next,
//     when an allocation requests occurs:
//       buffers[next_alloc_buffer] is sent to DMA,
//       next_alloc_buffer is set to !next_alloc_buffer,
//       next_alloc_record is set to zero, yielding state (1b), and
//       the allocation is handled as always, returning the new buffers[next_alloc_buffer][next_alloc_record]
//
//   (2b) If a DMA operation is in progress, then:
//     buffers[next_alloc_buffer] are used
//     buffers[!next_alloc_buffer] are being written to card
//     allocations fail because no records are free
//     when the DMA finishes:
//       buffers[!next_alloc_buffer] are no longer needed and, with on further action, state (2a) applies

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
		puts("LOG: Card full");
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

	printf("LOG: Start epoch %" PRIu16 " at sector %" PRIu32 "\n", epoch, next_write_sector);
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

	// If some records have been committed to the current sector, we must wipe the rest of the records and write out the sector.
	if (next_alloc_record) {
		while (next_alloc_record < RECORDS_PER_SECTOR) {
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

static log_record_t *log_alloc_impl(void) {
	// Sanity check.
	if (state != LOG_STATE_OK && state != LOG_STATE_OVERFLOW) {
		return 0;
	}

	// Check for full card.
	if (next_write_sector == sd_sector_count()) {
		state = LOG_STATE_CARD_FULL;
		puts("LOG: Card full");
		return 0;
	}

	// Check if we can do this the easy way, described by cases 1 in the state machine.
	if (next_alloc_record < RECORDS_PER_SECTOR) {
		return &buffers[next_alloc_buffer][next_alloc_record];
	}

	// Check for case 2a in the state machine.
	if (!sd_write_multi_busy()) {
		if (!sd_write_multi_sector(buffers[next_alloc_buffer])) {
			state = LOG_STATE_SD_ERROR;
			return 0;
		}
		next_alloc_buffer = !next_alloc_buffer;
		next_alloc_record = 0;
		++next_write_sector;
		state = LOG_STATE_OK;
		return &buffers[next_alloc_buffer][next_alloc_record];
	}

	// Buffer is full.
	state = LOG_STATE_OVERFLOW;
	return 0;
}

log_record_t *log_alloc(void) {
	log_record_t *rec = log_alloc_impl();

	if (rec) {
		uint32_t now = rdtsc();
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

	// If the current sector is not yet full, accept the record but do nothing special with it.
	if (++next_alloc_record < RECORDS_PER_SECTOR) {
		return;
	}

	if (sd_write_multi_busy()) {
		// DMA transfer already in progress, presumably for the other sector buffer.
		// We must therefore have been in state 1b before, and now we are in state 2b.
		// There is nothing to do at all until the DMA transfer finishes.
		return;
	} else {
		// DMA transfer not in progress.
		// We must therefore have been in state 1a before, and we can start a DMA transfer for the just-finished sector, moving to state 1b.
		if (!sd_write_multi_sector(buffers[next_alloc_buffer])) {
			state = LOG_STATE_SD_ERROR;
			return;
		}
		next_alloc_buffer = !next_alloc_buffer;
		next_alloc_record = 0;
		++next_write_sector;
		return;
	}
}

