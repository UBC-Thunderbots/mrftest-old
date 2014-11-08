/**
 * \defgroup LOG Data Logging Functions
 *
 * \brief These functions handle queueing up log records and writing them to the SD card for later review.
 *
 * A logging client calls \ref log_alloc which allocates a log buffer, if one is avaiable.
 * The client then fills the buffer before submitting it with \ref log_queue.
 * The log buffers are written to the SD card in order of submission.
 * Neither \ref log_alloc nor \ref log_queue will ever block.
 *
 * @{
 */

#include "log.h"
#include "dma.h"
#include "sdcard.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <queue.h>
#include <stddef.h>
#include <stdio.h>
#include <task.h>
#include <unused.h>

#define NUM_BUFFERS 16U
#define RECORDS_PER_SECTOR (SD_SECTOR_SIZE / LOG_RECORD_SIZE)

_Static_assert(sizeof(log_record_t) == LOG_RECORD_SIZE, "log_record_t is not LOG_RECORD_SIZE!");
_Static_assert((512U % LOG_RECORD_SIZE) == 0U, "log_record_t is not a perfect fraction of 512 bytes (an SD card sector) long!");

typedef struct {
	log_record_t records[RECORDS_PER_SECTOR];
} log_sector_t;

_Static_assert(sizeof(log_sector_t) == SD_SECTOR_SIZE, "log_sector_t is not SD_SECTOR_SIZE!");

static log_state_t state = LOG_STATE_UNINITIALIZED;
static uint16_t epoch;
static QueueHandle_t free_queue, write_queue;
static dma_memory_handle_t buffer_handles[NUM_BUFFERS];
static log_sector_t *filling_sector;
static unsigned int next_fill_record;
static unsigned int total_records;

static void log_writeout_task(void *param) {
	// Shovel records.
	uint32_t sector = (uint32_t) param;
	log_sector_t *data;
	do {
		// Receive a submitted record.
		BaseType_t rc = xQueueReceive(write_queue, &data, portMAX_DELAY);
		assert(rc == pdTRUE); // Receive can never fail because we wait forever.

		// If this was a real record (not the null pointer signifying shutdown) and nothing has failed yet, write it out to the SD card.
		if (data && state /* Non-atomic OK because only this task writes */ == LOG_STATE_OK) {
			if (sd_write(sector, data)) {
				++sector;
				if (sector == sd_sector_count()) {
					__atomic_store_n(&state, LOG_STATE_CARD_FULL, __ATOMIC_RELAXED);
				}
			} else {
				__atomic_store_n(&state, LOG_STATE_SD_ERROR, __ATOMIC_RELAXED);
				iprintf("Log: SD error writing sector %" PRIu32 "\r\n", sector);
			}
		}

		// Put the record back on the free queue.
		rc = xQueueSend(free_queue, &data, 0U);
		assert(rc == pdTRUE); // Send can never fail because free_queue is NUM_BUFFERS long or, in case of null, log_deinit has already sucked out all the real buffers.
	} while (data);

	// We have been asked to shut down, by means of a null pointer being sent over the write queue.
	// We have already replied by putting the null pointer back on the free queue, so just die.
	vTaskDelete(0);
}

/**
 * \brief Returns the current state of the logging subsystem.
 *
 * \return the current state
 */
log_state_t log_state(void) {
	return __atomic_load_n(&state, __ATOMIC_RELAXED);
}

/**
 * \brief Initializes the logging subsystem.
 *
 * \pre The SD card must already be initialized.
 *
 * \retval true initialization succeeded and the logging subsystem is ready to log data
 * \retval false initialization failed and \ref log_alloc will always return null
 */
bool log_init(void) {
	// Clear variables.
	filling_sector = 0;
	next_fill_record = 0U;
	total_records = 0U;

	// Allocate the log buffers.
	for (unsigned int i = 0U; i != NUM_BUFFERS; ++i) {
		buffer_handles[i] = dma_alloc(sizeof(log_sector_t));
	}

	// Sanity check.
	assert(state == LOG_STATE_UNINITIALIZED);

	// Binary search for the first empty sector.
	uint32_t next_write_sector;
	{
		uint32_t low = 0U, high = sd_sector_count();
		while (low != high && sd_status() == SD_STATUS_OK) {
			uint32_t probe = (low + high) / 2U;
			log_sector_t *buffer = dma_get_buffer(buffer_handles[0U]);
			if (!sd_read(probe, buffer)) {
				state = LOG_STATE_SD_ERROR;
				return false;
			}
			if (buffer->records[0U].magic == LOG_MAGIC_TICK) {
				low = probe + 1U;
			} else {
				high = probe;
			}
		}
		next_write_sector = low;
	}

	// Check if the card is completely full.
	if (next_write_sector == sd_sector_count()) {
		state = LOG_STATE_CARD_FULL;
		return false;
	}

	// Compute the epoch: previous sector’s epoch + 1 (if first empty sector is not first sector), or 1 (if first empty sector is first sector).
	if (next_write_sector > 0U) {
		log_sector_t *buffer = dma_get_buffer(buffer_handles[0U]);
		if (!sd_read(next_write_sector - 1U, buffer)) {
			state = LOG_STATE_SD_ERROR;
			return false;
		}
		epoch = buffer->records[0U].epoch + 1U;
	} else {
		epoch = 1U;
	}

	// Erase the card from the start sector to the end of the card.
	// Do this ahead of time so we won’t get stuck doing a long erase as part of the first write when time actually matters.
	if (!sd_erase(next_write_sector, sd_sector_count() - next_write_sector)) {
		return false;
	}

	// Create all the FreeRTOS IPC objects.
	free_queue = xQueueCreate(NUM_BUFFERS, sizeof(log_sector_t *));
	write_queue = xQueueCreate(NUM_BUFFERS, sizeof(log_sector_t *));
	assert(free_queue && write_queue);

	// Push all the buffers into the free queue.
	for (size_t i = 0U; i != NUM_BUFFERS; ++i) {
		log_sector_t *buffer = dma_get_buffer(buffer_handles[i]);
		BaseType_t rc = xQueueSend(free_queue, &buffer, 0U);
		assert(rc == pdTRUE); // Send can never fail because we are sending NUM_BUFFERS into a fresh NUM_BUFFERS-sized queue.
	}

	// Report status.
	printf("Start epoch %" PRIu16 " at sector %" PRIu32 " ", epoch, next_write_sector);
	state = LOG_STATE_OK;

	// Launch the writeout task.
	{
		BaseType_t rc = xTaskCreate(&log_writeout_task, "log-writeout", 512U, (void *) next_write_sector, PRIO_TASK_LOG_WRITEOUT, 0);
		assert(rc == pdPASS);
	}

	return true;
}

/**
 * \brief Deinitializes the logging subsystem.
 */
void log_shutdown(void) {
	// Sanity check.
	if (state != LOG_STATE_OK && state != LOG_STATE_CARD_FULL) {
		return;
	}

	// Remove all the free buffers from the queue.
	for (size_t i = 0U; i != NUM_BUFFERS; ++i) {
		log_sector_t *ptr;
		BaseType_t rc = xQueueReceive(free_queue, &ptr, portMAX_DELAY);
		assert(rc == pdTRUE); // Receive can never fail because we wait forever.
	}

	// Free all the buffers.
	for (size_t i = 0U; i != NUM_BUFFERS; ++i) {
		dma_free(buffer_handles[i]);
		buffer_handles[i] = 0;
	}

	// Send a null pointer into the write queue, signalling the writeout task to terminate.
	{
		log_sector_t *null = 0;
		BaseType_t rc = xQueueSend(write_queue, &null, 0U);
		assert(rc == pdTRUE); // Send can never fail because we have eaten all the buffers, so nothing can possibly be in either queue.
	}

	// Wait for the writeout task to send the null pointer back on the free queue, signalling that it is terminating.
	{
		log_sector_t *ptr;
		BaseType_t rc = xQueueReceive(free_queue, &ptr, portMAX_DELAY);
		assert(rc == pdTRUE); // Receive can never fail because we wait forever.
		assert(!ptr);
	}

	// Destroy the FreeRTOS IPC objects.
	vQueueDelete(free_queue);
	vQueueDelete(write_queue);

	// Update state.
	state = LOG_STATE_UNINITIALIZED;
}

/**
 * \brief Allocates a log record for the application to write into.
 *
 * The newly allocated record will have its epoch and timestamp fields filled in before it is returned.
 * The application must fill the magic field and any record-type-specific data.
 *
 * \return the log record, or null on failure
 */
log_record_t *log_alloc(void) {
	if (log_state() != LOG_STATE_OK) {
		return 0;
	}

	if (!filling_sector) {
		next_fill_record = 0U;
		BaseType_t rc = xQueueReceive(free_queue, &filling_sector, 0U);
		if (rc != pdTRUE) {
			filling_sector = 0;
			iprintf("Log: out of buffers at %u.\r\n", total_records);
			return 0;
		}
	}

	log_record_t *rec = &filling_sector->records[next_fill_record];
	rec->epoch = epoch;
	rec->time = xTaskGetTickCount();
	return rec;
}

/**
 * \brief Submits a log record for writeout.
 *
 * \param[in] record the log record, which must have previously been allocated with \ref log_alloc
 */
void log_queue(log_record_t *record) {
	assert(record == &filling_sector->records[next_fill_record]);
	++next_fill_record;
	++total_records;
	if (next_fill_record == RECORDS_PER_SECTOR) {
		BaseType_t rc = xQueueSend(write_queue, &filling_sector, 0U);
		assert(rc == pdTRUE); // Send can never fail because we only ever allocate NUM_BUFFERS buffers, and write_queue is NUM_BUFFERS long.
		filling_sector = 0;
	}
}

/**
 * @}
 */
