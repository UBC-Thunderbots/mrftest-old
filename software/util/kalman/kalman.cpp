#include "util/kalman/kalman.h"
#include "util/time.h"
#include <cmath>

Kalman::Kalman(bool angle, double measure_std, double accel_std) : last_control(0.0), sigma_m(measure_std), sigma_a(accel_std), h(1, 2), p(2, 2, Matrix::InitFlag::IDENTITY), state_estimate(2, 1, Matrix::InitFlag::ZEROES), is_angle(angle) {
	last_measurement_time.tv_sec = 0;
	last_measurement_time.tv_nsec = 0;
	// %the state measurement operator
	// H=[1 0];(;
	h(0, 0) = 1.0;
	h(0, 1) = 0.0;
}

Matrix Kalman::gen_g_mat(double timestep) const {
	// %the acceleration to state vector (given an acceleration, what is the state
	// %update)
	// G=[timestep.^2/2; timestep];
	Matrix G(2, 1);
	G(0, 0) = timestep * timestep / 2;
	G(1, 0) = timestep;
	return G;
}

Matrix Kalman::gen_q_mat(double timestep) const {
	const Matrix &g = gen_g_mat(timestep);
	// %The amount of uncertainty gained per step
	const Matrix &q = g * ~g * sigma_a * sigma_a;
	return q;
}

Matrix Kalman::gen_f_mat(double timestep) const {
	// %the state update matrix (get next state given current one
	// F=[1 timestep;0 1];
	Matrix f(2, 2);
	f(0, 0) = 1.0;
	f(0, 1) = timestep;
	f(1, 0) = 0.0;
	f(1, 1) = 1.0;
	return f;
}

// predict forward one step with fixed control parameter
void Kalman::predict_step(double timestep, double control, Matrix &state_predict, Matrix &p_predict) const {
	const Matrix &f = gen_f_mat(timestep);
	const Matrix &g = gen_g_mat(timestep);
	const Matrix &q = gen_q_mat(timestep);
	state_predict = f * state_predict + g * control;
	p_predict = f * p_predict * ~f + q;
}

// get an estimate of the state and covariance at prediction_time, outputs passed by reference
void Kalman::predict(timespec prediction_time, Matrix &state_predict, Matrix &p_predict) const {
	state_predict = state_estimate;
	p_predict = p;
	timespec current_time = last_measurement_time;
	double current_control = last_control;

	for (std::deque<ControlInput>::const_iterator inputs_itr = inputs.begin(); inputs_itr != inputs.end() && timespec_cmp(inputs_itr->time, prediction_time) < 0; ++inputs_itr) {
		predict_step(timespec_to_double(timespec_sub(inputs_itr->time, current_time)), current_control, state_predict, p_predict);
		current_time = inputs_itr->time;
		current_control = inputs_itr->value;
	}
	predict_step(timespec_to_double(timespec_sub(prediction_time, current_time)), current_control, state_predict, p_predict);
	if (is_angle) {
		state_predict(0, 0) -= 2 * M_PI * std::round(state_predict(0, 0) / 2 / M_PI);
	}
}

// get an estimate of the state at prediction_time
Matrix Kalman::predict(timespec prediction_time) const {
	Matrix state_predict;
	Matrix p_predict;
	predict(prediction_time, state_predict, p_predict);
	return state_predict;
}

// this should generate an updated state, as well as clean up all the inputs since the last measurement
// until this current one
void Kalman::update(double measurement, timespec measurement_time) {
	Matrix state_priori;
	Matrix p_priori;
	predict(measurement_time, state_priori, p_priori);

	// %how much does the guess differ from the measurement
	double residual = measurement - (h * state_priori)(0, 0);
	if (is_angle) {
		residual -= 2 * M_PI * std::round(residual / 2 / M_PI);
	}

	// %The kalman update calculations
	const Matrix &kalman_gain = (p_priori * ~h) / (((h * p_priori * ~h)(0, 0)) + sigma_m * sigma_m);
	state_estimate = state_priori + kalman_gain * residual;

	if (is_angle) {
		state_estimate(0, 0) -= 2 * M_PI * std::round(state_estimate(0, 0) / 2 / M_PI);
	}
	p = (Matrix(2, 2, Matrix::InitFlag::IDENTITY) - kalman_gain * h) * p_priori;

	last_measurement_time = measurement_time;

	// we should clear the control inputs for times before last_measurement_time because they are unneeded
	// and we don't want memory to explode
	while (!inputs.empty() && timespec_cmp(inputs.front().time, last_measurement_time) <= 0) {
		last_control = inputs.front().value;
		inputs.pop_front();
	}
}

double Kalman::get_control(timespec control_time) const {
	double current_control = last_control;

	for (std::deque<ControlInput>::const_iterator inputs_itr = inputs.begin(); inputs_itr != inputs.end() && timespec_cmp(inputs_itr->time, control_time) <= 0; ++inputs_itr) {
		current_control = inputs_itr->value;
	}
	return current_control;
}

void Kalman::add_control(double input, timespec input_time) {
	// the new control input overwrites all actions that were scheduled for later times
	// this allows us, for instance, to change our mind about future control sequences
	while (!inputs.empty() && timespec_cmp(inputs.back().time, input_time) >= 0) {
		inputs.pop_back();
	}
	if (timespec_cmp(input_time, last_measurement_time) <= 0) {
		last_control = input;
	} else {
		inputs.push_back(ControlInput(input_time, input));
	}
}

