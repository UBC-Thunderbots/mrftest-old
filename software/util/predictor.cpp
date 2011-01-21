#include "util/predictor.h"
#include "geom/angle.h"
#include "leastsquares/leastsquares.h"
#include "util/time.h"
#include "util/timestep.h"
#include <iostream>
#include <cmath>

//TODO 1. accompensate for delay in real mode, and not to in simulator mode
// TODO 2. access to control noise: as if our robot command and relative robot positions
// TODO 3. accompensate for glitches when "clipping" angles

namespace {
	const int MAX_DEGREE = 3;
	const int NUM_OLD_POSITIONS = 30;
	const double ANGLE_UPPER_BOUND = 1000*M_PI;
}

Predictor::Predictor(bool angle) : angle(angle), initialized(false), lock_delta(0.0) {
	// Record current time.
	timespec_now(last_datum_timestamp);
	lock_timestamp = last_datum_timestamp;

	// Fill history data with zeroes.
	vhistory.setlength(NUM_OLD_POSITIONS);
	dhistory.setlength(NUM_OLD_POSITIONS);
	//weiguess(0,0);ghts.setlength(NUM_OLD_POSITIONS);
	fmatrix.setlength(NUM_OLD_POSITIONS, MAX_DEGREE + 1);
	for (int i = 0; i < NUM_OLD_POSITIONS; i++) {
		vhistory(i) = 0.0;
		dhistory(i) = 1.0 / TIMESTEPS_PER_SECOND;
	}

	// Allocate space for the approximants.
	approxv.setlength(MAX_DEGREE + 1);
	
	filter.set_availability(true);

}

Predictor::~Predictor() {
}

double Predictor::value(double delta, unsigned int deriv) const {
	delta += lock_delta;
	double v = 0;
	//original
	/*for (int i = MAX_DEGREE; i >= static_cast<int>(deriv); --i) { 
		v = v * delta + approxv(i);
	}
	if (angle && !deriv) {
		v = angle_mod(v);
	}
	return v;*/
	matrix guess = filter.predict(delta);
	if(deriv == 0){
		v = guess(0,0);
	} else if(deriv == 1) {
		v = guess(1,0);
		//std::cout << "velocity is:" << v << std::endl ;
	}else {} // TODO some time, deal with acceleration

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
		//update();
		// or use kalman filter
		filter.update(value, timespec_to_double(ts));

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
		if(filter.is_available() && value > ANGLE_UPPER_BOUND){
			value -= ANGLE_UPPER_BOUND;
			filter.reset_angle(ANGLE_UPPER_BOUND);
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
	//update();
	// or use the kalman predictor
	//filter.new_control(3,);
	filter.update(value, timespec_to_double(ts));
	//TODO deal with angle prediction
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

