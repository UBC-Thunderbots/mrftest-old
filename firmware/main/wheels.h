#ifndef WHEELS_H
#define WHEELS_H

#include "log.h"
#include <stdint.h>

typedef struct drive_struct drive_t;

#define WHEELS_ENCODER_COUNTS_PER_REV 1440U
#define WHEELS_POLE_PAIRS 8U
#define WHEELS_HALL_COUNTS_PER_REV (WHEELS_POLE_PAIRS * 6U)
#define WHEELS_ENCODER_COUNTS_PER_HALL_COUNT (WHEELS_ENCODER_COUNTS_PER_REV / WHEELS_HALL_COUNTS_PER_REV)
#define WHEELS_SPEED_CONSTANT 374.0f // rpm per volt—EC45 datasheet
#define WHEELS_VOLTS_PER_RPM (1.0f / WHEELS_SPEED_CONSTANT) // volts per rpm
#define WHEELS_VOLTS_PER_RPT (WHEELS_VOLTS_PER_RPM / 60.0f / CONTROL_LOOP_HZ) // volts per rpt, rpt=revolutions per tick
#define WHEELS_VOLTS_PER_ENCODER_COUNT (WHEELS_VOLTS_PER_RPT / (float) WHEELS_ENCODER_COUNTS_PER_REV) // volts per encoder count

/**
 * \ingroup WHEELS
 *
 * \brief The modes the wheels can be in.
 */
typedef enum {
	WHEELS_MODE_COAST, ///< The commutation pattern is controlled by the user.
	WHEELS_MODE_BRAKE, ///< All low-side drivers are on while all high-side drivers are off, thus regeneratively braking.
	WHEELS_MODE_OPEN_LOOP, ///< Motor commutation is controlled by Hall sensors, with the user providing PWM values.
	WHEELS_MODE_CLOSED_LOOP, ///< Motor commutation is controlled by Hall sensors, and PWM values are provided by the control loop.
} wheels_mode_t;

void wheels_tick(const drive_t *drive, log_record_t *record);

#endif

