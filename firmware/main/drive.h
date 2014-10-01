#ifndef DRIVE_H
#define DRIVE_H

typedef struct drive_struct drive_t;

#include "wheels.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * \ingroup DRIVE
 *
 * \brief The control parameters for robot driving.
 */
struct drive_struct {
	wheels_mode_t wheels_mode; ///< The mode to run the wheels.
	unsigned int dribbler_power; ///< The PWM level to run the dribbler.
	bool charger_enabled; ///< Whether to charge the capacitors.
	bool discharger_enabled; ///< Whether to discharge the capacitors.
	int16_t setpoints[4U]; ///< The drive setpoints.
	uint8_t data_serial; ///< The serial number of the data, used to detect when new setpoints are issued.
};

const drive_t *drive_read_get(void);
void drive_read_put(void);
drive_t *drive_write_get(void);
void drive_write_put(void);

#endif
