#include "util/kalman/matrix.h"
#include <deque>

#ifndef KALMAN_H_
#define KALMAN_H_

struct ControlInput {
	ControlInput(double t, double v) : time(t), value(v) {}
	double time;
	double value;
};

class kalman {
	public:
		kalman(bool angle = false, double measure_std = 1.3e-3, double accel_std = 2.0);
		void predict(double prediction_time, matrix &state_predict, matrix &P_predict) const;
		matrix predict(double prediction_time) const;
		void update(double measurement, double measurement_time);
		void add_control(double input, double input_time);
		double get_control(double control_time) const;
		void set_availability(bool bit) { available = bit; }
		bool is_available() { return available; }

	private:
		void predict_step(double timestep, double control, matrix &state_predict, matrix &P_predict) const;
		double last_measurement_time;
		double last_control;
		double sigma_m;
		double sigma_a;
		std::deque<ControlInput> inputs;
		matrix gen_F_mat(double timestep) const;
		matrix gen_Q_mat(double timestep) const;
		matrix gen_G_mat(double timestep) const;
		matrix H;
		matrix P;
		matrix state_estimate;
		bool available;
		bool is_angle;
};

#endif

