#ifndef UTIL_KALMAN_KALMAN_H
#define UTIL_KALMAN_KALMAN_H

#include "util/matrix.h"
#include <deque>

struct ControlInput {
	ControlInput(double t, double v) : time(t), value(v) {}
	double time;
	double value;
};

class Kalman {
	public:
		Kalman(bool angle = false, double measure_std = 1.3e-3, double accel_std = 2.0);
		void predict(double prediction_time, Matrix &state_predict, Matrix &p_predict) const;
		Matrix predict(double prediction_time) const;
		void update(double measurement, double measurement_time);
		void add_control(double input, double input_time);
		double get_control(double control_time) const;
		void set_availability(bool bit) { available = bit; }
		bool is_available() const { return available; }

	private:
		void predict_step(double timestep, double control, Matrix &state_predict, Matrix &p_predict) const;
		double last_measurement_time;
		double last_control;
		double sigma_m;
		double sigma_a;
		std::deque<ControlInput> inputs;
		Matrix gen_f_mat(double timestep) const;
		Matrix gen_q_mat(double timestep) const;
		Matrix gen_g_mat(double timestep) const;
		Matrix h;
		Matrix p;
		Matrix state_estimate;
		bool available;
		bool is_angle;
};

#endif

