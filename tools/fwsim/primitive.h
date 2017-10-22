#ifndef PRIMITIVES_PRIMITIVE_H
#define PRIMITIVES_PRIMITIVE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief The information about a movement sent from the host computer.
 */
typedef struct {
	/**
	 * \brief The four primary data parameters.
	 */
	int16_t params[4];

	/**
	 * \brief Whether the robot has been ordered to drive slowly.
	 */
	bool slow;

	/**
	 * \brief The extra data byte.
	 */
	uint8_t extra;
} primitive_params_t;

/**
 * \brief The definition of a movement primitive.
 *
 * The movement primitive framework ensures that, even in the face of multiple
 * threads, no more than one of the functions below is called at a time.
 * Therefore, it is safe to, for example, access global variables in both the
 * \ref start and the \ref tick functions.
 */

//void primitive_init(void);
//void primitive_start(unsigned int primitive, const primitive_params_t *params);
//void primitive_tick(log_record_t *log);
//bool primitive_is_direct(unsigned int primitive);
//unsigned int get_primitive_index();
//bool primitive_params_are_equal(primitive_params_t* params1,primitive_params_t* params);

#endif
