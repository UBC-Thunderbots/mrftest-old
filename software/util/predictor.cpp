#include "util/predictor.h"
#include "geom/angle.h"
#include "leastsquares/leastsquares.h"
#include "util/time.h"
#include "util/timestep.h"
#include <cmath>
/*
#include <queue>
#include <utility>
#include "util/matrix.h"

namespace
{
	const int CONSTANT = 0;
	matrix state_estimate;
	matrix covariance_estimate;
	matrix state_model;
	
	matrix control_model;
	matrix observation_model;
	matrix process_covariance;
	matrix observation_covariance;
}

Predictor::Prediction(bool angle) : angle(angle), initialized(false), lock_delta(0.0)
{
	
}

Predictor::~Predictor() {}

pair<matrix, matrix> Prediction::Predict(double delta_time, double acceleration = 0.0)
{
	state_model(0, 0) = 1;
	state_mode(0, 1) = delta_time;
	state_model(1, 0) = 1;
	state_model(1, 1) = 0;
	control_vector(0, 0) = acceleration;
	control_model(0, 0) = 0.5*delta_time*delta_time;
	control_model(1, 0) = delta_time;
	priori_estimate = state_model*state_estimate + control_model*control_vector;
	priori_covariance = state_model*covariance_estimate*(~state_model) + process_covariance;
	return make_pair(priori_estimate,priori_covariance);
}

void Prediction::Update(double observation, double frame_delay, double acceleration = 0.0)
{
	pair<matrix,matrix> pa = Predict(frame_delay, acceleration);
	state_estimate = pa.first; covariance_estimate = pa.second;
	innovation_residual = observation - observation_model*state_estimate;
	innovation_covariance = observation_model*priori_covariance*(~observation_model) + observation_covariance;
	kalman_gain = priori_covariance*(~observation_model)/innovation_covariance(0,0); // assuming innovation_covariance is a scalar
	state_estimate = state_estimate + kalman_gain*innovation_residual;
	covariance_estimate = (I - kalman_gain*observation_model)*covariance_estimate;
}
*/
namespace {
	const int MAX_DEGREE = 3;
	const int NUM_OLD_POSITIONS = 30;
}

Predictor::Predictor(bool angle) : angle(angle), initialized(false), lock_delta(0.0) {
	// Record current time.
	timespec_now(last_datum_timestamp);
	lock_timestamp = last_datum_timestamp;

	// Fill history data with zeroes.
	vhistory.setlength(NUM_OLD_POSITIONS);
	dhistory.setlength(NUM_OLD_POSITIONS);
	weights.setlength(NUM_OLD_POSITIONS);
	fmatrix.setlength(NUM_OLD_POSITIONS, MAX_DEGREE + 1);
	for (int i = 0; i < NUM_OLD_POSITIONS; i++) {
		vhistory(i) = 0.0;
		dhistory(i) = 1.0 / TIMESTEPS_PER_SECOND;
	}

	// Allocate space for the approximants.
	approxv.setlength(MAX_DEGREE + 1);
}

Predictor::~Predictor() {
}

double Predictor::value(double delta, unsigned int deriv) const {
	delta += lock_delta;
	double v = 0;
	for (int i = MAX_DEGREE; i >= static_cast<int>(deriv); --i) {
		v = v * delta + approxv(i);
	}
	if (angle && !deriv) {
		v = angle_mod(v);
	}
	return v;
}

double Predictor::value(const timespec &ts, unsigned int deriv) const {
	timespec delta;
	timespec_sub(ts, lock_timestamp, delta);
	return value(timespec_to_double(delta), deriv);
}

void Predictor::lock_time(const timespec &ts) {
	lock_timestamp = ts;
	timespec diff;
	timespec_sub(ts, last_datum_timestamp, diff);
	lock_delta = timespec_to_double(diff);
}

void Predictor::add_datum(double value, const timespec &ts) {
	// If this is our first datum, do a clear instead.
	if (!initialized) {
		// Remember that we're initialized.
		initialized = true;

		// Mark the current time as the most recent datum stamp.
		last_datum_timestamp = ts;

		// Fill the current value into all history cells.
		// This means that the object appears to be stationary.
		for (int i = 0; i < NUM_OLD_POSITIONS; ++i) {
			vhistory(i) = value;
			dhistory(i) = 1.0 / TIMESTEPS_PER_SECOND;
		}

		// Update the predictions.
		update();

		// Don't do any of the rest.
		return;
	}

	// Compute delta time and update stamp.
	timespec diff;
	timespec_sub(ts, last_datum_timestamp, diff);
	last_datum_timestamp = ts;
	double delta_time = timespec_to_double(diff);

	// In the angle case, move the new value close to the previous value.
	if (angle) {
		while (std::fabs(value - vhistory(0)) > M_PI + 1e-9) {
			if (value > vhistory(0)) {
				value -= 2 * M_PI;
			} else {
				value += 2 * M_PI;
			}
		}
	}

	// Add new data to history list.
	for (int i = NUM_OLD_POSITIONS - 1; i; i--) {
		vhistory(i) = vhistory(i - 1);
		dhistory(i) = dhistory(i - 1);
	}
	vhistory(0) = value;
	dhistory(0) = delta_time;

	// Update the predictions.
	update();
}

void Predictor::clear() {
	initialized = false;
}

void Predictor::update() {
	// Build the matrices.
	double dd = 0; // dd is always nonpositive
	for (int i = 0; i < NUM_OLD_POSITIONS; i++) {
		if (i) {
			dd -= dhistory(i - 1);
		}
		// Weights are 1 / (number of AI time steps from the present + 1).
		// Data further in the past will affect our predictions less.
		weights(i) = 1.0 / (1 - dd * TIMESTEPS_PER_SECOND);
		fmatrix(i, 0) = 1;
		for (unsigned int j = 1; j < MAX_DEGREE + 1; j++) {
			fmatrix(i, j) = fmatrix(i, j - 1) * dd;
		}
	}

	// If this is an angle predictor and the values are getting too big, reduce them to prevent floating point precision loss.
	if (angle) {
		while (std::fabs(vhistory(0)) > 64 * M_PI) {
			double shift = vhistory(0) > 0 ? -64 * M_PI : 64 * M_PI;
			for (int i = 0; i < NUM_OLD_POSITIONS; i++) {
				vhistory(i) += shift;
			}
		}
	}

	// Perform the regressions.
	buildgeneralleastsquares(vhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxv);
}

