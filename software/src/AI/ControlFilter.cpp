#include "AI/ControlFilter.h"

MoveFilter::MoveFilter(
		const std::vector<double>& ka,
		const std::vector<double>& kb) : a(ka), b(kb) {
	delayed.resize(a.size() - 1, 0);
}

void MoveFilter::clear() {
	for (unsigned int i = 0; i < delayed.size(); i++) {
		delayed[i] = 0;
	}
}

double MoveFilter::process(double input) {
	const unsigned int n = delayed.size();
	for (unsigned int i = 0; i < n; i++) {
		input -= a[i+1] * delayed[i] / a[0];
	}
	double output = b[0] * input;
	for (unsigned int i = 0; i < n; i++) {
		output += b[i+1] * delayed[i] / a[0];
	}
	for (unsigned int i = n - 1; i > 0; i--) {
		delayed[i] = delayed[i-1];
	}
	//input = input - a[1] * delayed[0] / a[0] - a[2] * delayed[1] / a[0];
	//output = b[0] * input + b[1] * delayed[0] / a[0] + b[2] * delayed[1] / a[0];
	//delayed[1] = delayed[0];
	//delayed[0] = input;
	return output;
}

PID::PID(double Kp, double Ki, double Kd, double decay) : Kp(Kp), Ki(Ki), Kd(Kd), decay(decay), integral(0), hasPrev(false) {
}

void PID::clear() {
	integral = 0;
	hasPrev = false;
}

double PID::process(double error) {
	integral = integral * decay + error;
	double answer = Kp * error + Ki * integral + (hasPrev ? Kd * (error - prev) : 0);
	prev = error;
	hasPrev = true;
	return answer;
}

