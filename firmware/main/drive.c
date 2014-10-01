/**
 * \defgroup DRIVE Driving Parameters Functions
 *
 * \brief These functions handle synchronizing between producers and consumers of robot drive parameters.
 *
 * There can be at most one producer and one consumer running at a time.
 *
 * @{
 */

#include "drive.h"
#include "tbuf.h"
#include <assert.h>
#include <limits.h>

static drive_t buffers[3U] = {
	[0U ... 2U] = {
		.wheels_mode = WHEELS_MODE_COAST,
		.dribbler_power = 0U,
		.charger_enabled = false,
		.discharger_enabled = false,
		.setpoints = { 0, 0, 0, 0 },
	}
};
static tbuf_t buffers_ctl = TBUF_INIT;
static unsigned int current_read_buffer = UINT_MAX, current_write_buffer = UINT_MAX;

/**
 * \brief Locks and returns a pointer to the most recently written drive data.
 *
 * While the drive data is locked, it will not be modified and represents an atomic snapshot for the consumer to examine at leisure.
 * The consumer must release the data with drive_read_put when it is finished with it.
 *
 * \return the data
 */
const drive_t *drive_read_get(void) {
	assert(current_read_buffer == UINT_MAX);
	current_read_buffer = tbuf_read_get(&buffers_ctl);
	return &buffers[current_read_buffer];
}

/**
 * \brief Releases the drive data lock taken by \ref drive_read_get.
 *
 * Once the lock is released, the application should not access the data as it may be changing.
 */
void drive_read_put(void) {
	assert(current_read_buffer != UINT_MAX);
	tbuf_read_put(&buffers_ctl, current_read_buffer);
	current_read_buffer = UINT_MAX;
}

/**
 * \brief Locks and returns a pointer to the next drive data buffer to fill.
 *
 * The producer must fill the locked buffer and then commit it with \ref drive_write_put.
 *
 * \return the buffer to fill
 */
drive_t *drive_write_get(void) {
	assert(current_write_buffer == UINT_MAX);
	current_write_buffer = tbuf_write_get(&buffers_ctl);
	return &buffers[current_write_buffer];
}

/**
 * \brief Commits data written into a buffer returned by \ref drive_write_get.
 *
 * Once the lock is released, the producer must not touch the buffer as it may be being consumed.
 */
void drive_write_put(void) {
	assert(current_write_buffer != UINT_MAX);
	tbuf_write_put(&buffers_ctl, current_write_buffer);
	current_write_buffer = UINT_MAX;
}

/**
 * @}
 */
