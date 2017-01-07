#include "kalman.h"
#include <stdio.h>


void print_mat(unsigned int rows, unsigned int cols, float X[rows][cols]){
  for(unsigned int i = 0; i < rows; i++){
    for(unsigned int j = 0; j < cols; j++){
      printf("%9.6f    ", X[i][j]);
    }
    printf("\n");
  }
}


const float eye_3[3][3] = 
{{1.0, 0.0, 0.0},
 {0.0, 1.0, 0.0},
 {0.0, 0.0, 1.0}};

const float F_x[3][3] =
{{1.0, DEL_T, 0.0},
 {0.0, 1.0,   0.0},
 {0.0, 0.0,   0.0}};
const float F_y[3][3] =
{{1.0, DEL_T, 0.0},
 {0.0, 1.0,   0.0},
 {0.0, 0.0,   0.0}};
const float F_t[3][3] =
{{1.0, DEL_T, 0.0},
 {0.0, 1.0,   0.0},
 {0.0, 0.0,   0.0}};

const float B_x[3][1] =
{{DEL_T_2/2},
 {DEL_T},
 {1.0}};
const float B_y[3][1] =
{{DEL_T_2/2},
 {DEL_T},
 {1.0}};
const float B_t[3][1] =
{{DEL_T_2/2},
 {DEL_T},
 {1.0}};

static float P_x[3][3] = 
{{0.0, 0.0, 0.0},
 {0.0, 2.0, 0.0},
 {0.0, 0.0, 2.0}};
static float P_y[3][3] = 
{{0.0, 0.0, 0.0},
 {0.0, 2.0, 0.0},
 {0.0, 0.0, 2.0}};
static float P_t[3][3] = 
{{0.0, 0.0,  0.0},
 {0.0, 10.0, 0.0},
 {0.0, 0.0,  10.0}};
 
const float Q_x[3][3] =
{{DEL_T*0.0,    DEL_T*0.0,    DEL_T*0.0001},
 {DEL_T*0.0,    DEL_T*0.0001, DEL_T*0.02},
 {DEL_T*0.0001, DEL_T*0.02,   DEL_T*4.0}};
const float Q_y[3][3] =
{{DEL_T*0.0,    DEL_T*0.0,    DEL_T*0.0001},
 {DEL_T*0.0,    DEL_T*0.0001, DEL_T*0.02},
 {DEL_T*0.0001, DEL_T*0.02,   DEL_T*4.0}};
const float Q_t[3][3] =
{{DEL_T*0.0,    DEL_T*0.0,    DEL_T*0.0013},
 {DEL_T*0.0,    DEL_T*0.0025, DEL_T*0.5},
 {DEL_T*0.0013, DEL_T*0.5,    DEL_T*100.0}};

const float H_x[2][3] =
{{0.0, 1.0, 0.0},
 {0.0, 0.0, 1.0}};
const float H_y[2][3] =
{{0.0, 1.0, 0.0},
 {0.0, 0.0, 1.0}};
const float H_t[1][3] =
{{0.0, 1.0, 0.0}};

const float H_x_cam[3][3] =
  {{1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0}};
const float H_y_cam[3][3] =
  {{1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0}};
const float H_t_cam[2][3] =
  {{1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0}};

const float R_x[2][2] =
{{VAR_SPEED_X, 0.0},
 {0.0, VAR_ACC_X}};
const float R_y[2][2] =
{{VAR_SPEED_Y, 0.0},
 {0.0, VAR_ACC_Y}};
const float R_t[1][1] = {{(VAR_SPEED_T + VAR_GYRO) / 4}};

const float R_x_cam[3][3] =
  {{VAR_CAM_X, 0.0, 0.0},
  {0.0, VAR_SPEED_X, 0.0},
  {0.0, 0.0, VAR_ACC_X}};
const float R_y_cam[3][3] =
  {{VAR_CAM_Y, 0.0, 0.0},
  {0.0, VAR_SPEED_Y, 0.0},
  {0.0, 0.0, VAR_ACC_Y}};
const float R_t_cam[2][2] =
  {{VAR_CAM_T, 0.0},
  {0.0, (VAR_SPEED_T + VAR_GYRO) / 4}};

static float S_x[2][2] = {{0}};
static float S_y[2][2] = {{0}};
static float S_t[1][1] = {{0}};


static float S_x_cam[3][3] = {{0}};
static float S_y_cam[3][3] = {{0}};
static float S_t_cam[2][2] = {{0}};


static float K_x[3][3] = {{0}};
static float K_y[3][3] = {{0}};
static float K_t[3][3] = {{0}};

static float x_x[3] = {0};
static float x_y[3] = {0};
static float x_t[3] = {0};

void kalman_step(dr_data_t *state, kalman_data_t *kalman_state) {

  
  // Temp values for the x and u contributions during the predict step.
  float temp_x[3] = {0};
  float temp_u[3] = {0};

  // Temp values for the prediction step.
  float temp_P[3][3]; 
  float temp_P2[3][3]; 
  float y_x[3];
  float y_y[3];
  float y_t[2];
  float temp_z[3];

  // Prediction step.
  // Predict x.
  matrix_mult(temp_x, 3, x_x, 3, F_x);
  matrix_mult(temp_u, 3, &(kalman_state->x_accel), 1, B_x); 
  vectorAdd(temp_x, temp_u, 3);  
  vectorCopy(x_x, temp_x, 3);
  // Predict y.
  matrix_mult(temp_x, 3, x_y, 3, F_y);
  matrix_mult(temp_u, 3, &(kalman_state->y_accel), 1, B_y); 
  vectorAdd(temp_x, temp_u, 3);  
  vectorCopy(x_y, temp_x, 3);
  // Predict t.
  matrix_mult(temp_x, 3, x_t, 3, F_t);
  matrix_mult(temp_u, 3, &(kalman_state->t_accel), 1, B_t); 
  vectorAdd(temp_x, temp_u, 3);  
  vectorCopy(x_t, temp_x, 3);
 
  // Predict x covariance. 
  mm_mult(3, 3, 3, F_x, (const float (*)[3])P_x, temp_P);
  mm_mult_t(3, 3, 3, (const float (*)[3])temp_P, F_x, P_x);
  mm_add(3, 3, P_x, Q_x); 
  // Predict y covariance. 
  mm_mult(3, 3, 3, F_y, (const float (*)[3])P_y, temp_P);
  mm_mult_t(3, 3, 3, (const float (*)[3])temp_P, F_y, P_y);
  mm_add(3, 3, P_y, Q_y); 
  // Predict t covariance. 
  mm_mult(3, 3, 3, F_t, (const float (*)[3])P_t, temp_P);
  mm_mult_t(3, 3, 3, (const float (*)[3])temp_P, F_t, P_t);
  mm_add(3, 3, P_t, Q_t); 
  


    if(kalman_state->new_camera_data){

	  kalman_state->new_camera_data = false;

	  
	  	  // Calculate Kalman gain for x.
	  	  mm_mult(3, 3, 3,  H_x_cam, (const float (*)[3])P_x, temp_P);
	  	  mm_mult_t(3, 3, 3, (const float (*)[3])temp_P, H_x_cam, S_x_cam);
	  	  mm_add(3, 3, S_x_cam, R_x_cam);
	  	  mm_inv(3, S_x_cam);
	  	  mm_mult_t(3, 3, 3, (const float (*)[3])P_x, H_x_cam, temp_P);
	  	  mm_mult(3, 3, 3, (const float (*)[3])temp_P, (const float (*)[3])S_x_cam, K_x); // K_x[3][3]
	  	  // Calculate Kalman gain for y.
	  	  mm_mult(3, 3, 3,  H_y_cam, (const float (*)[3])P_y, temp_P);
	  	  mm_mult_t(3, 3, 3, (const float (*)[3])temp_P, H_y_cam, S_y_cam);
	  	  mm_add(3, 3, S_y_cam, R_y_cam);
	      mm_inv(3, S_y_cam);
	  	  mm_mult_t(3, 3, 3, (const float (*)[3])P_y, H_y_cam, temp_P);
	      mm_mult(3, 3, 3, (const float (*)[3])temp_P, (const float (*)[3])S_y_cam, K_y); // K_y[3][3]
	  	  // Calculate Kalman gain for t.
	  	  mm_mult(2, 3, 3,  H_t_cam, (const float (*)[3])P_t, temp_P);
	  	  mm_mult_t(2, 2, 3, (const float (*)[3])temp_P, H_t_cam, S_t_cam);
	  	  mm_add(2, 2, S_t_cam, R_t_cam);
	  	  mm_inv(2, S_t_cam);
	  	  mm_mult_t(3, 2, 3, (const float (*)[3])P_t, H_t_cam, temp_P);
	  	  mm_mult(3, 2, 2, (const float (*)[3])temp_P, (const float (*)[3])S_t_cam, K_t); // K_t[3][2]

	  	  // Add sensor residual to x.
	  	  matrix_mult(y_x, 3, x_x, 3, H_x_cam);
	  	  temp_z[0] = kalman_state->cam_x;
	  	  temp_z[1] = kalman_state->wheels_x;
	  	  temp_z[2] = kalman_state->accelerometer_x;
	  	  vectorSub(temp_z, y_x, 3);
	  	  vectorCopy(y_x, temp_z, 3);
	  	  matrix_mult(temp_x, 3, y_x, 3, (const float (*)[3])K_x);
	  	  vectorAdd(x_x, temp_x, 3);
	  	  // Add sensor residual to y.
	  	  matrix_mult(y_y, 3, x_y, 3, H_y_cam);
	  	  temp_z[0] = kalman_state->cam_y;
	  	  temp_z[1] = kalman_state->wheels_y;
	  	  temp_z[2] = kalman_state->accelerometer_y;


	  	  vectorSub(temp_z, y_y, 3);
	  	  vectorCopy(y_y, temp_z, 3);
	  	  matrix_mult(temp_x, 3, y_y, 3, (const float (*)[3])K_y);
	  	  vectorAdd(x_y, temp_x, 3);
	  	  // Add sensor residual to t.
	  	  matrix_mult(y_t, 2, x_t, 3, H_t_cam);


          float cam_t = kalman_state->cam_t;
	  	  if(cam_t >= y_t[0] ){
	  		float cam_sub = cam_t - 2*M_PI;
	  		if((cam_sub - y_t[0])*(cam_sub - y_t[0]) >= (cam_t - y_t[0])*(cam_t - y_t[0])){
	  			temp_z[0] = cam_t;
	  		}else{
	  			temp_z[0] = cam_sub;
	  		}
	  	  }else{
			float cam_plus = cam_t + 2*M_PI;
			if((cam_plus - y_t[0])*(cam_plus - y_t[0]) >= (cam_t - y_t[0])*(cam_t - y_t[0])){
				temp_z[0] = cam_t;
			}else{
				temp_z[0] = cam_plus;
			}
	  	  }



	  	  temp_z[1] = (kalman_state->gyro + kalman_state->wheels_t) / 2;
	  	  vectorSub(temp_z, y_t, 2);
	  	  vectorCopy(y_t, temp_z, 2);
	  	  matrix_mult(temp_x, 3, y_t, 2, (const float (*)[3])K_t);
	  	  vectorAdd(x_t, temp_x, 3);
	  	  if (x_t[0] < -2*M_PI) {
	  		x_t[0] += 2*M_PI;
	  	  }
	  	  else if (x_t[0] > 2*M_PI) {
	  		x_t[0] -= 2*M_PI;
	  	  }

	  // Update the x covariance.
	  mm_mult(3, 3, 3, (const float (*)[3])K_x, H_x_cam, temp_P);
	  mm_sub(3, 3, eye_3, (const float (*)[3])temp_P, temp_P2);

	  mm_mult(3,3,3,(const float (*)[3])temp_P2,(const float (*)[3])P_x, temp_P);

	  mm_copy(3,3,P_x,temp_P);
	  // Update the y covariance.
	  mm_mult(3, 3, 3, (const float (*)[3])K_y, H_y_cam, temp_P);
	  mm_sub(3, 3, eye_3, (const float (*)[3])temp_P, temp_P2);

	  mm_mult(3,3,3,(const float (*)[3])temp_P2,(const float (*)[3])P_y, temp_P);

	  mm_copy(3,3,P_y,temp_P);
	  // Update the t covariance.
	  mm_mult(3, 2, 3, (const float (*)[3])K_t, H_t_cam, temp_P);
	  mm_sub(3, 3, eye_3, (const float (*)[3])temp_P, temp_P2);

	  mm_mult(3,3,3,(const float (*)[3])temp_P2,(const float (*)[3])P_t, temp_P);

	  mm_copy(3,3,P_t,temp_P);

  }else{


	  // TODO Perform correction step.
	  // Calculate Kalman gain for x.
	  mm_mult(2, 3, 3,  H_x, (const float (*)[3])P_x, temp_P);
	  mm_mult_t(2, 2, 3, (const float (*)[3])temp_P, H_x, S_x);
	  mm_add(2, 2, S_x, R_x);
	  mm_inv(2, S_x);
	  mm_mult_t(3, 2, 3, (const float (*)[3])P_x, H_x, temp_P);
	  mm_mult(3, 2, 2, (const float (*)[3])temp_P, (const float (*)[3])S_x, K_x); 
	  
	  // K_x[3][2]
	  // Calculate Kalman gain for y.
	  mm_mult(2, 3, 3,  H_y, (const float (*)[3])P_y, temp_P);
	  mm_mult_t(2, 2, 3, (const float (*)[3])temp_P, H_y, S_y);
	  mm_add(2, 2, S_y, R_y);
	  mm_inv(2, S_y);
	  mm_mult_t(3, 2, 3, (const float (*)[3])P_y, H_y, temp_P);
	  mm_mult(3, 2, 2, (const float (*)[3])temp_P, (const float (*)[3])S_y, K_y); // K_y[3][2]
	  // Calculate Kalman gain for t.
	  mm_mult(1, 3, 3,  H_t, (const float (*)[3])P_t, temp_P);
	  mm_mult_t(1, 1, 3, (const float (*)[3])temp_P, H_t, S_t);
	  mm_add(1, 1, S_t, R_t);
	  mm_inv(1, S_t);
	  mm_mult_t(3, 1, 3, (const float (*)[3])P_t, H_t, temp_P);
	  mm_mult(3, 1, 1, (const float (*)[3])temp_P, (const float (*)[3])S_t, K_t); // K_t[3][1]

	  // Add sensor residual to x.
	  matrix_mult(y_x, 2, x_x, 3, H_x);
	  temp_z[0] = kalman_state->wheels_x;
	  temp_z[1] = kalman_state->accelerometer_x;
	  vectorSub(temp_z, y_x, 2);
	  vectorCopy(y_x, temp_z, 2);
	  matrix_mult(temp_x, 3, y_x, 2, (const float (*)[3])K_x);
	  vectorAdd(x_x, temp_x, 3);
	  // Add sensor residual to y.
	  matrix_mult(y_y, 2, x_y, 3, H_y);
	  temp_z[0] = kalman_state->wheels_y;
	  temp_z[1] = kalman_state->accelerometer_y;
	  vectorSub(temp_z, y_y, 2);
	  vectorCopy(y_y, temp_z, 2);
	  matrix_mult(temp_x, 3, y_y, 2, (const float (*)[3])K_y);
	  vectorAdd(x_y, temp_x, 3);
	  // Add sensor residual to t.
	  matrix_mult(y_t, 1, x_t, 3, H_t);
	  temp_z[0] = (kalman_state->gyro + kalman_state->wheels_t) / 2;
	  vectorSub(temp_z, y_t, 1);
	  vectorCopy(y_t, temp_z, 1);
	  matrix_mult(temp_x, 3, y_t, 1, (const float (*)[3])K_t);
	  vectorAdd(x_t, temp_x, 3);
	  if (x_t[0] < -2*M_PI) {
		x_t[0] += 2*M_PI;
	  }
	  else if (x_t[0] > 2*M_PI) {
		x_t[0] -= 2*M_PI;
	  }

	  // Update the x covariance.
	  mm_mult(3, 2, 3, (const float (*)[3])K_x, H_x, temp_P);
	  mm_sub(3, 3, eye_3, (const float (*)[3])temp_P, temp_P2);

	  mm_mult(3,3,3,(const float (*)[3])temp_P2,(const float (*)[3])P_x, temp_P);

	  mm_copy(3,3,P_x,temp_P);
	  // Update the y covariance.
	  mm_mult(3, 2, 3, (const float (*)[3])K_y, H_y, temp_P);
	  mm_sub(3, 3, eye_3, (const float (*)[3])temp_P, temp_P2);

	  mm_mult(3,3,3,(const float (*)[3])temp_P2,(const float (*)[3])P_y, temp_P);

	  mm_copy(3,3,P_y,temp_P);
	  // Update the t covariance.
	  mm_mult(3, 1, 3, (const float (*)[3])K_t, H_t, temp_P);
	  mm_sub(3, 3, eye_3, (const float (*)[3])temp_P, temp_P2);

	  mm_mult(3,3,3,(const float (*)[3])temp_P2,(const float (*)[3])P_t, temp_P);

	  mm_copy(3,3,P_t,temp_P);
  }

  // Update the dead reckoning state.
  state->x = x_x[0];
  state->y = x_y[0];
  state->angle = x_t[0];
  state->vx = x_x[1];
  state->vy = x_y[1];
  state->avel = x_t[1];
  /*
  state->x = kalman_state->cam_x;
  state->y = kalman_state->cam_y;
  state->angle = kalman_state->cam_t;
  */

}

void kalman_step_x(dr_data_t *state, kalman_data_t *kalman_state) {
  // Temp values for the x and u contributions during the predict step.
  float temp_x[3] = {0};
  float temp_u[3] = {0};

  // Temp values for the prediction step.
  float temp_P[3][3];
  float temp_P2[3][3]; 
  float y_x[3];
  float temp_z[3];

  // Prediction step.
  // Predict x.
  matrix_mult(temp_x, 3, x_x, 3, F_x);
  matrix_mult(temp_u, 3, &(kalman_state->x_accel), 1, B_x); 
  vectorAdd(temp_x, temp_u, 3);  
  vectorCopy(x_x, temp_x, 3);
 
  // Predict x covariance. 
  mm_mult(3, 3, 3, F_x, (const float (*)[3])P_x, temp_P);
  mm_mult_t(3, 3, 3, (const float (*)[3])temp_P, F_x, P_x);
  mm_add(3, 3, P_x, Q_x);

          // TODO Perform correction step.
	  // Calculate Kalman gain for x.
	  mm_mult(2, 3, 3,  H_x, (const float (*)[3])P_x, temp_P);
	  mm_mult_t(2, 2, 3, (const float (*)[3])temp_P, H_x, S_x);
	  mm_add(2, 2, S_x, R_x);

	  mm_inv(2, S_x);
	  mm_mult_t(3, 2, 3, (const float (*)[3])P_x, H_x, temp_P);
	  mm_mult(3, 2, 2, (const float (*)[3])temp_P, (const float (*)[3])S_x, K_x); 

	  // Add sensor residual to x.
	  matrix_mult(y_x, 2, x_x, 3, H_x);


	  temp_z[0] = kalman_state->wheels_x;
	  temp_z[1] = kalman_state->accelerometer_x;

	  vectorSub(temp_z, y_x, 2);

	  vectorCopy(y_x, temp_z, 2);
	  matrix_mult(temp_x, 3, y_x, 2, (const float (*)[3])K_x);
	  vectorAdd(x_x, temp_x, 3);

	
	  // Update the x covariance.
	  mm_mult(3, 2, 3, (const float (*)[3])K_x, H_x, temp_P);
	  mm_sub(3, 3, eye_3, (const float (*)[3])temp_P, temp_P2);

	  mm_mult(3,3,3,(const float (*)[3])temp_P2,(const float (*)[3])P_x, temp_P);

	  mm_copy(3,3,P_x,temp_P);
  // Update the dead reckoning state.
  state->x = x_x[0];
  state->vx = x_x[1];
}
