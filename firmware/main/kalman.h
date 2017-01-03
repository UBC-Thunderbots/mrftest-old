#ifndef KALMAN_H
#define KALMAN_H
#include "physics.h"
#include "dr.h"

#define VAR_CAM_X   9.0e-6f
#define VAR_CAM_Y   9.0e-6f
#define VAR_CAM_T   9.0e-6f
#define VAR_ACC_X   0.0047f
#define VAR_ACC_Y   0.0135f
#define VAR_GYRO    1.0034e-4f
#define VAR_SPEED_X 0.0025
#define VAR_SPEED_Y 0.0029
#define VAR_SPEED_T 0.0019
#define DEL_T       TICK_TIME
#define DEL_T_2     TICK_TIME*TICK_TIME



extern const float eye_3[3][3];

extern const float F_x[3][3];
extern const float F_y[3][3];
extern const float F_t[3][3];

extern const float B_x[3][1];
extern const float B_y[3][1];
extern const float B_t[3][1];

extern const float Q_x[3][3];
extern const float Q_y[3][3];
extern const float Q_t[3][3];

extern const float H_x[2][3];
extern const float H_y[2][3];
extern const float H_t[1][3];

extern const float R_x[2][2];
extern const float R_y[2][2];
extern const float R_t[1][1];

void kalman_step(dr_data_t *state, kalman_data_t *kalman_state);
void kalman_step_x(dr_data_t *state, kalman_data_t *kalman_state);

#endif
