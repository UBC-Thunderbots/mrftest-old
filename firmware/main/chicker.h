#ifndef CHICKER_H
#define CHICKER_H

#include <stdbool.h>

/**
 * \ingroup CHICKER
 *
 * \brief The available devices.
 */
typedef enum {
	CHICKER_KICK, ///< Performs a straight, flat kick.
	CHICKER_CHIP, ///< Performs a chip kick up into the air.
} chicker_device_t;

/**
 * \brief The voltage above which the charged LED illuminates.
 */
#define CHICKER_CHARGED_THRESHOLD 35.0f

/**
 * \brief The voltage above which the safe discharge pulse generator generates discharge pulses.
 */
#define CHICKER_DISCHARGE_THRESHOLD 25.0f

void chicker_init(void);
void chicker_shutdown(void);
void chicker_discharge(bool discharge);
void chicker_fire(chicker_device_t device, unsigned int width);
void chicker_auto_arm(chicker_device_t device, unsigned int width);
void chicker_auto_disarm(void);
bool chicker_auto_armed(void);
bool chicker_auto_fired_test_clear(void);
void chicker_tick(void);
void chicker_tick_fast(void);

#endif
