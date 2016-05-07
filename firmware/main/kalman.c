#include "kalman.h"
#include "physics.h"
#include "physics.c"
#include <math.h>
#include <stdint.h>

// State transition matrix
static const float A_mat[6][6]=
{
		{1.0000, 0.0000, 0.0000, TICK_TIME, 0.0000, 0.0000},
		{0.0000, 1.0000, 0.0000, 0.0000, TICK_TIME, 0.0000},
		{0.0000, 0.0000, 1.0000, 0.0000, 0.0000, TICK_TIME},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000}
};

// Input matrix
static const float B_mat[6][3]=
{
		{0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000/ROBOT_RADIUS}
};

// Process noise covariance matrix
static const float Q_mat[6][6]=
{
		{1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000}
};

// Measurement noise covariance matrix
static const float R_mat[6][6]=
{
		{1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000}
};

//  Observation model maps voltage inputs to states
static const float H_mat[6][6]=
{
		{1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000}
};

// Error covariance matrix
static float P_mat[6][6]=
{
		{1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000}
};

//Kalman gain
static float K_mat[6][6]=
{
		{1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000}
};

//Identity
static float I_mat[6][6]=
{
		{1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000},
		{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000}
};

void kalman_step(const float current_state[6]) {
	float next_state[6]; //unforced contribution
	float next_state2[6]; //input contribution
	float p_mat[6][6];
	float k_mat[6][6];
	float z[6];
	float y[6];

	// next_state = A*state + B*u
	matrix_mult(next_state, 6, current_state, 6, A_mat);
	matrix_mult(next_state2, 6, current_state, 6, B_mat);
	vectorAdd(next_state, next_state2);
	next_state2 = next_state;

	// p = A*P*A' + Q
	matrix_mult_matrix(p_mat, 6, 6, 6, A_mat, P_mat);
	matrix_mult_matrix_t(p_mat, 6, 6, 6, p_mat, A_mat);
	matrixAdd(6, 6, p_mat, Q_mat);

	matrix_mult(z, 6, next_state, 6, H_mat);
	rand_vector(y, 6);
	vectorAdd(z, y);

	matrix_mult_matrix(K_mat, 6, 6, 6, p_mat, H_mat);
	matrix_mult_matrix(k_mat, 6, 6, 6, H_mat, p_mat);
	matrix_mult_matrix_t(k_mat, 6, 6, 6, p_mat, H_mat);
	matrixAdd(6, 6, k_mat, R_mat);
	matrix_inv(k_mat, 6);
	matrix_mult_matrix(K_mat, 6, 6, 6, K_mat, k_mat);

	matrix_mult(next_state2, 6, y, 6, K_mat);
	vectorAdd(next_state, next_state2);
	current_state = next_state;

	matrix_mult_matrix(P_mat, 6, 6, 6, K_mat, H_mat);
	matrixSub(P_mat, 6, 6, I_mat, P_mat);
	matrix_mult_matrix(P_mat, 6, 6, 6, P_mat, p_mat);
}

void rand_vector(float* vector, int len) {
	for(int i=0;i<len;i++) {
		vector[i] = TICK_TIME*randn(0,1);
	}
}
//random normal with mean mu and standard deviation sigma
double randn(mu, sigma) {
	double W, U1, U2, K;
	static float X1, X2;
	static bool call = true;
	if(call) {
		call = !call;
		return mu+sigma*X2;
	}
	do{
		U1 = -1+(double)rand()*2;
		U2 = -1+(double)rand()*2;
		W = pow(U1, 2) + pow(U2, 2);
	}while(W>=0 || W==0);
	K = sqrt(-2*log(W)/W);
	X1 = U1*K;
	X2 = U2*K;
	return mu+sigma*X1;
}
