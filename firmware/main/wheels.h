#ifndef WHEELS_H
#define WHEELS_H

#include <stdint.h>

/**
 * \brief The modes the wheels can be in
 */
typedef enum {
	WHEEL_MODE_COAST,
	WHEEL_MODE_BRAKE,
	WHEEL_MODE_OPEN_LOOP,
	WHEEL_MODE_CLOSED_LOOP,
} wheel_mode_t;


typedef struct {
	wheel_mode_t mode;
	int16_t setpoints[4];
} wheel_ctx_t;

/**
 * \brief the current wheel settings
 *
 */
extern wheel_ctx_t wheel_context;

/**
 * \brief update the controllers for new wheel settings
 */
void wheel_update_ctx();

/**
 * \brief Runs the wheels
 */
void wheels_tick();

#endif

