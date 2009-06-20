#include "AI/ControlFilter.h"

#include <algorithm>

MoveFilter::MoveFilter(
		const std::vector<double>& ka,
		const std::vector<double>& kb) : a(ka), b(kb) {
	delayed.resize(a.size() - 1, 0);
}

void MoveFilter::clear() {
	std::fill(delayed.begin(), delayed.end(), 0);
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
	std::copy_backward(&delayed[0], &delayed[n - 1], &delayed[1]);
	delayed[0] = input;
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

