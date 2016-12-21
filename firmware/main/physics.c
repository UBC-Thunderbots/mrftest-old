#include "physics.h"
#include "adc.h"
#include "encoder.h"
#include <math.h>
#include <stdint.h>


//Wheel angles for these matricies are (55, 135, 225, 305) degrees
//these matrices may be derived as per omnidrive_kiart paper


//This matrix is a unitless matrix which takes the force exerted
//on the floor by each of the four wheels and converts it into forces
//within the robot domain, the third component is is newtons and not 
//torque as the matrix is unitless (multiply by ROBOT_RADIUS to unnormalize)
//the transpose of this matrix is the velocity coupling matrix and can convert
//speeds in the robot coordinates into linear wheel speeds
static const float force4_to_force3_mat[3][4]=
{
		{-0.8192, -0.7071, 0.7071, 0.8192},
		{ 0.5736, -0.7071,-0.7071, 0.5736},
		{ 1.0000,  1.0000, 1.0000, 1.0000} 
};

// Transformation matricies to convert a 4 velocity to
// a 3 velocity (derived as pinv(force4_to_force3^t)
// this is also the transpose of force3_to_force4 mat
static const float speed4_to_speed3_mat[3][4]=
{
	{-0.3498,-0.3019, 0.3019, 0.3498},
	{ 0.3904,-0.3904,-0.3904, 0.3904},
	{ 0.2761, 0.2239, 0.2239, 0.2761}
};

//mass vector (consists linear robot mass and interial mass)
const float ROBOT_MASS[3] = {ROBOT_POINT_MASS, ROBOT_POINT_MASS, ROT_MASS};

const float MAX_VEL[3] = {MAX_X_V, MAX_Y_V, MAX_T_V*ROBOT_RADIUS};
const float MAX_ACC[3] = {MAX_X_A, MAX_Y_A, MAX_T_A*ROBOT_RADIUS};

//contention vector, where in forces will simply consume power.
static const float contention_vector[4] = {-0.4621, 0.5353, -0.5353, 0.4621};

/**
 * \brief Multiplies a matrix by a vector.
 *
 * \param[out] lhs the result, \p matrix * \p rhs
 * \param[in] lhs_len the size of the output vector
 * \param[in] rhs the vector to multiply
 * \param[in] rhs_len the size of the input vector
 * \param[in] matrix the matrix to multiply
 */
void matrix_mult(float* lhs, int lhs_len, const float* rhs,int rhs_len, const float matrix[lhs_len][rhs_len]) {
	for(int j=0;j<lhs_len;++j) {
		lhs[j]=0.0f;
		for(int i=0;i<rhs_len;++i) {
			lhs[j] += matrix[j][i]*rhs[i];
		}
	}
}

/**
 * \brief Multiplies a matrix's transpose by a vector.
 *
 * \param[out] lhs the result, \p matrixT * \p rhs
 * \param[in] lhs_len the size of the output vector
 * \param[in] rhs the vector to multiply
 * \param[in] rhs_len the size of the input vector
 * \param[in] matrix the matrix to multiply
 */
void matrix_mult_t(float* lhs, int lhs_len, const float* rhs, int rhs_len, const float matrix[rhs_len][lhs_len]) {
	for(int j=0;j<lhs_len;++j) {
		lhs[j]=0.0f;
		for(int i=0;i<rhs_len;++i) {
			lhs[j] += matrix[i][j]*rhs[i];
		}
	}
}

/**
 * \brief Multiplies two matrices.
 *
 * \param[out] matrix_out the result 
 * \param[in] lm_rows number of rows of left matrix
 * \param[in] lm_cols number of cols of right matrix
 * \param[in] rm_rows number of rows of right matrix
 * \param[in] rm_cols number of cols of right matrix
 * \param[in] lmatrix left matrix to multiply
 * \param[in] rmatrix right matrix to multiply 
 */
void mm_mult(int lm_rows, int rm_rows, int rm_cols, const float lmatrix[lm_rows][rm_rows], const float rmatrix[rm_rows][rm_cols], float matrix_out[lm_rows][rm_cols]) {
  int i;
  int j;
  int k;
  // NOTE rm_rows and lm_cols must be equal!
  float temp;
  for(i = 0; i < rm_cols; i++) {
    for(j = 0; j < lm_rows; j++) {
      temp = 0;
      for(k = 0; k < rm_rows; k++) {
        temp += lmatrix[j][k]*rmatrix[k][i];
      }
      matrix_out[j][i] = temp;     
    }
  }
}

/**
 * \brief Multiplies one matrix by another's transpose.
 *
 * \param[out] matrix_out the result 
 * \param[in] lm_rows number of rows of left matrix
 * \param[in] lm_cols number of cols of left matrix
 * \param[in] rm_rows number of rows of right matrix
 * \param[in] rm_cols number of cols of right matrix
 * \param[in] lmatrix left matrix to multiply
 * \param[in] rmatrix right matrix to transpose multiply 
 */
void mm_mult_t(int lm_rows, int rm_rows, int rm_cols, const float lmatrix[lm_rows][rm_rows], const float rmatrix[rm_rows][rm_cols], float matrix_out[lm_rows][rm_rows]) {
  int i;
  int j;
  int k;
  // NOTE rm_rows and lm_cols must be equal!
  float temp;
  for(i = 0; i < rm_rows; i++) {
    for(j = 0; j < lm_rows; j++) {
      temp = 0;
      for(k = 0; k < rm_cols; k++) {
        temp += lmatrix[j][k]*rmatrix[i][k];
      }
      matrix_out[j][i] = temp;     
    }
  }
}

/**
 * \brief Adds matrix B to A, and stores in A. 
 *
 * \param[in] nrows number of rows
 * \param[in] ncols number of cols
 * \param[in, out] a destination matrix 
 * \param[in] b source matrix 
 */

void mm_add(int nrows, int ncols, float a[nrows][ncols], const float b[nrows][ncols]) {
  int i, j;
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < ncols; j++) {
      a[i][j] = a[i][j] + b[i][j];
    }
  }
}
/**
 * \brief Subtracts matrix B from A, and stores in C. 
 *
 * \param[out] c the result 
 * \param[in] nrows number of rows
 * \param[in] ncols number of cols
 * \param[in] a source matrix 1
 * \param[in] b source matrix 2 
 */

void mm_sub(int nrows, int ncols, const float a[nrows][ncols], const float b[nrows][ncols], float c[nrows][ncols]) {
  int i, j;
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < ncols; j++) {
      c[i][j] = a[i][j] - b[i][j];
    }
  }
}

/**
 * \brief Inverts a 2x2 or a 1x1 matrix. 
 *
 * \param[in, out] a the result 
 * \param[in] n number of rows and columns
 */

void mm_inv(int n, float a[n][n]) {
  float det, temp;
  
  switch(n) {
    case (1):
      a[0][0] = 1 / a[0][0];
      break;
    case (2):
      det = 1 / (a[0][0]*a[1][1] - a[0][1]*a[1][0]);
      temp = a[0][0];
      a[0][0] = a[1][1] / det;
      a[1][1] = temp / det;
      a[0][1] = -a[0][1] / det;
      a[1][0] = -a[1][0] / det;
      break;
    default:
      break;
  }
}

/**
 * \ingroup Physics
 *
 * \brief performs the unitless conversion of wheel speeds into robot speeds
 * 
 * \param[in] the 4 wheel speeds
 * \param[out] the 3 robot speeds in the same units
 */
void speed4_to_speed3(const float speed4[4], float speed3[3]) {
	matrix_mult(speed3,3,speed4,4, speed4_to_speed3_mat);
}

/**
 * \ingroup Physics
 *
 * \brief performs the unitless conversion of the robots speeds into wheel speeds
 * 
 * \param[in] the robot speeds in x,y,theta*R coordinates
 * \param[out] the robot wheel speeds in the same units as input
 */
void speed3_to_speed4(const float speed3[3], float speed4[4]) {
	matrix_mult_t(speed4, 4, speed3, 3, force4_to_force3_mat);
}


/**
 * \ingroup Physics
 *
 * \brief implements the vector transfrom of A=A-B
 *
 * \param[in,out] the A vector
 * \param[in] the B vector
 * \param[in] the length of the vectors
 *
 */
void vectorSub(float *a,const float *b, int len) {
	for(int i=0;i<len;i++) {
		a[i] = a[i]-b[i];
	}
}

/**
 * \ingroup Physics
 *
 * \brief implements the vector transform of A=A+B
 *
 * \param[in,out] the A vector
 * \param[in] the B vector
 * \param[in] the length of the vectors
 *
 */
void vectorAdd(float *a,const float *b, int len) {
	for(int i = 0; i < len; i++) {
		a[i] = a[i] + b[i];
	}
}

/**
 * \ingroup Physics
 *
 * \brief copies source b into destination a. 
 *
 * \param[in,out] the destination vector
 * \param[in] the source vector
 * \param[in] the length of the vectors
 *
 */
void vectorCopy(float *a, const float *b, int len) {
  int i;
  for(i = 0; i < len; i++) {
    a[i] = b[i]; 
  }
}

/**
 * \ingroup Physics
 *
 * \brief perfoms 2D cartesian to polar coords
 *
 * \param[in,out] input vector in cartesian coordinates to replace with polar
 *
 */
void Cart2Pol(float vec[2]) {
	float temp = sqrtf(vec[0]*vec[0] + vec[1]*vec[1]);
	vec[1] = atan2f(vec[1], vec[0]);
	vec[0] = temp;
}

/**
 * \ingroup Physics
 *
 * \brief perfoms 2D polar to cartesian
 *
 * \param[in,out] input vector in polar to replace with cartesian
 *
 */
void Pol2Cart(float vec[2]) {
	float temp = vec[0]*cosf(vec[1]);
	vec[1] = vec[0]*sinf(vec[1]);
	vec[0] = temp;
}

/**
 * \ingroup Physics
 *
 * \brief converts a 2D cartesian velocity into a polar velocity given a polar location
 *
 * \param[in] location of velocity in polar coordinates
 * \param[in,out] velocity in cartesian coordinates to replace with polar
 */
void CartVel2Pol(float const loc[2], float vel[2]) {
	float temp = cosf(loc[1])*vel[0] + sinf(loc[1])*vel[1];
	vel[1] = -sinf(loc[1])/loc[0]*vel[0] + cosf(loc[1])/loc[0]*vel[1];
	vel[0] = temp;
}

/**
 * \ingroup Physics
 *
 * \brief converts a 2D polar velocity into a cartesian velocity given a polar location
 *
 * \param[in] location of velocity in polar coordinates
 * \param[in,out] velocity in polar coordinates to replace with cartesian
 */
void PolVel2Cart(float const loc[2], float vel[2]) {
	float temp = cosf(loc[1])*vel[0] - loc[0]*sinf(loc[1])*vel[1];
	vel[1] = sinf(loc[1])*vel[0] + loc[0]*cosf(loc[1])*vel[1];
	vel[0] = temp;
}

/**
 * \ingroup Physics
 *
 * \brief converts a 2D polar acceleration into linear acceleration
 *
 * \param[in] Polar Position
 * \param[in] Polar Velocity
 * \param[in] Polar Acceleration
 * \param[out] acceleration in cartesian coords
 */
void PolAcc2Cart(float const loc[2], float const vel[2], float const Pacc[2], float Cacc[2]) {
	float cosT = cosf(loc[1]);
	float sinT = sinf(loc[1]);
	Cacc[0] = cosT*Pacc[0] - loc[0]*sinT*Pacc[1] + vel[1] * (-2.0f*sinT*vel[0] - loc[0]*cosT*vel[1]);
	Cacc[1] = sinT*Pacc[0] + loc[0]*cosT*Pacc[1] + vel[1] * ( 2.0f*cosT*vel[0] - loc[1]*sinT*vel[1]);
}



/**
 * \ingroup Physics
 *
 * \brief implements 2D rotation matrix
 *
 * \param[in,out] the speed to rotate
 * \param[in] amount in radian to rotate
 */
void rotate(float speed[2], float angle) {
	float temp = cosf(angle)*speed[0] - sinf(angle)*speed[1];
	speed[1] = sinf(angle)*speed[0] + cosf(angle)*speed[1];
	speed[0]=temp;
}

/**
 * \ingroup Physics
 *
 * Implements the conversion between forces in the robot coordinate system and the 
 * Force per wheel. This is nominally equal to the speed3_to_speed4 conversion if
 * the center of mass of the robot coincides with the wheel center, however this is 
 * not the case and so when computing wheel forces this should be transform should
 * be used.
 *
 * \brief Implements the conversion from force in robot coordinates to Wheel force
 * 
 * \param[in] force in robot coordinates
 * \param[out] force to exert per wheel 
 */
void force3_to_force4(float force3[3], float force4[4]) {
	matrix_mult_t(force4, 4, force3, 3, speed4_to_speed3_mat);
}

/**
 * \ingroup Physics
 *
 * \brief compute the scaling constant to bring motor torques to maximum 
 *
 * \param[in] attempted torque to exert per wheel
 *
 * \return the amount by which to scale the torque vector to max it out
 */
float get_maximal_torque_scaling(const float torque[4]) {
	float acc_max = -INFINITY;
	float vapp_max = -INFINITY;
	for(int i=0;i<4;i++) {
		float volt = torque[i]*CURRENT_PER_TORQUE*PHASE_RESISTANCE;
		float back_emf = (float)encoder_speed(i)*QUARTERDEGREE_TO_VOLT;
		float appl_volt = fabsf(volt+back_emf);
		float max_app = fabsf(volt);
		if(max_app > acc_max) {
			acc_max = max_app;
		}
		if(appl_volt > vapp_max) {
			vapp_max = appl_volt;
		}
	}
	
	float slip_ratio = DELTA_VOLTAGE_LIMIT / acc_max;
	float emf_ratio = adc_battery() / vapp_max;

	return (emf_ratio > slip_ratio)?slip_ratio:emf_ratio; 
}


/**
 * \ingroup Physics
 *
 * \brief compute the scaling constant to bring robot acceleration to maximum
 *
 * \param[in] attempted linear acceleration in robot coordinates
 * \param[in] attempted angular acceleration
 *
 * \return amount by which to scale acceleration
 */
float get_maximal_accel_scaling(const float linear_accel[2], float angular_accel) {
	//first convert accelerations into consistent units
	//choose units of Force (N)
	float normed_force[3];
	normed_force[0]=linear_accel[0]*ROBOT_MASS[0];
	normed_force[1]=linear_accel[1]*ROBOT_MASS[1];
	normed_force[2]=angular_accel*ROBOT_MASS[2]*ROBOT_RADIUS;

	float wheel_force[4];
	force3_to_force4(normed_force, wheel_force);
	for(int i=0;i<4;++i) {
		wheel_force[i] *= WHEEL_RADIUS*GEAR_RATIO; //convert to motor torque
	}
	return get_maximal_torque_scaling(wheel_force);
}
