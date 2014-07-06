/**
 * \defgroup HALL Hall Sensor Functions
 *
 * \brief These functions handle measuring the speed of motors using the Hall sensors.
 *
 * @{
 */
#include "hall.h"
#include "icb.h"
#include <assert.h>

#define NUM_HALL_SENSORS 5U

static int16_t last_positions[NUM_HALL_SENSORS];
static int16_t speeds[NUM_HALL_SENSORS];

/**
 * \brief Initializes the Hall sensor speed measurement subsystem.
 */
void hall_init(void) {
	hall_tick(true);
}

/**
 * \brief Updates the current Hall sensor speed measurements.
 *
 * \param[in] dribbler \c true to also update the dribbler (which is updated more slowly), or \c false to update only the wheels
 */
void hall_tick(bool dribbler) {
	static int16_t new_positions[NUM_HALL_SENSORS];
	icb_receive(ICB_COMMAND_MOTORS_GET_HALL_COUNT, new_positions, sizeof(new_positions));
	for (unsigned int i = 0U; i != (dribbler ? NUM_HALL_SENSORS : (NUM_HALL_SENSORS - 1U)); ++i) {
		speeds[i] = new_positions[i] - last_positions[i];
		last_positions[i] = new_positions[i];
	}
}

/**
 * \brief Reads the speed of a motor from its most recent Hall sensor tick.
 *
 * \param[in] index the index of the Hall sensor to read
 *
 * \return the speed of the shaft
 */
int16_t hall_speed(unsigned int index) {
	assert(index < NUM_HALL_SENSORS);
	return speeds[index];
}

/**
 * @}
 */

