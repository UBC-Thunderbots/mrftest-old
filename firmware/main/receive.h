#ifndef RECEIVE_H
#define RECEIVE_H

struct receive_drive_struct;
typedef struct receive_drive_struct receive_drive_t;

#include "motor.h"
#include "wheels.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * \ingroup RECEIVE
 *
 * \brief The type of data decoded from a drive packet.
 */
struct receive_drive_struct {
	wheels_mode_t wheels_mode; ///< The mode to run the wheels.
	unsigned int dribbler_power; ///< The PWM level to run the dribbler.
	bool charger_enabled; ///< Whether to charge the capacitors.
	bool discharger_enabled; ///< Whether to discharge the capacitors.
	int16_t setpoints[4U]; ///< The drive setpoints.
	uint8_t data_serial; ///< The serial number of the data, used to detect when new setpoints are issued.
};

void receive_init(unsigned int index);
void receive_shutdown(void);
void receive_tick(void);
const receive_drive_t *receive_lock_latest_drive(void);
void receive_release_drive(void);
bool receive_drive_timeout(void);

#endif

