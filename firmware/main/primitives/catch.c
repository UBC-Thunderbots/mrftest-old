#include "catch.h"
#include "../control.h"
#include "../dr.h"
#include "../physics.h"
#include <math.h>
#include <unused.h>

#define CATCH_MAX_X_V (MAX_X_V/2)
#define CATCH_MAX_Y_V (MAX_Y_V/2)
#define CATCH_MAX_T_V (MAX_T_V/2)

#define CATCH_MAX_X_A (2.0f)
#define CATCH_MAX_Y_A (2.0f)
#define CATCH_MAX_T_A (20.0f)

static primitive_params_t catch_param;

        
/**
 * \brief Initializes the catch primitive.
 *
 * This function runs once at system startup.
 */
static void catch_init(void) {
}

/**
 * \brief Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a catch
 * movement.
 *
 * \param[in] params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 */
static void catch_start(const primitive_params_t *params) {        
        for( unsigned int i = 0; i < 4; i++ ){
                catch_param.params[i] = params->params[i];      
        }
        catch_param.slow = params->slow;
        catch_param.extra = params->extra;
}

/**
 * \brief Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * catch movement is already in progress.
 */
static void catch_end(void) {
}

/**
 * \brief Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */
static void catch_tick(log_record_t *UNUSED(log)) {
	dr_data_t data;
	float acc_target[3];
	float vx_target = catch_param.params[2]/1000.0f;
	float vx_target_abs;
	float vx_diff;

        // grab position, velocity measurement
        dr_get(&data);

	vx_target_abs = fabsf(vx_target);
	// simple velocity control
	// a PI controller would probably reduce the amount of oscillation
	vx_diff = vx_target-data.vx;
	if( vx_diff > vx_target_abs/2 ){
		acc_target[0] = CATCH_MAX_X_A;
	} else if( vx_diff > 0.02f ){
		acc_target[0] = CATCH_MAX_X_A/10;
	} else if( vx_diff < -vx_target_abs/2 ){
		acc_target[0] = -CATCH_MAX_X_A;
	} else if( vx_diff < -0.02f ){
		acc_target[0] = -CATCH_MAX_X_A/10;
	} else {
		acc_target[0] = 0.0f;
	}

        acc_target[1] = compute_accel_track_pos_1D(catch_param.params[1]/1000.0f, data.y, data.vy, CATCH_MAX_Y_V, CATCH_MAX_Y_A);
        acc_target[2] = compute_accel_track_pos_1D(catch_param.params[0]/100.0f, data.angle, data.avel, CATCH_MAX_T_V, CATCH_MAX_T_A);

        apply_accel(acc_target, acc_target[2]);


}

/**
 * \brief The catch movement primitive.
 */
const primitive_t CATCH_PRIMITIVE = {
	.direct = false,
	.init = &catch_init,
	.start = &catch_start,
	.end = &catch_end,
	.tick = &catch_tick,
};
