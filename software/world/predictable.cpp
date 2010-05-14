#include "geom/angle.h"
#include "leastsquares/leastsquares.h"
#include "world/predictable.h"
#include "world/timestep.h"
#include <cmath>

namespace {
	const int MAX_DEGREE = 3;
	const int NUM_OLD_POSITIONS = 10;
}

predictable::predictable() {
	// Fill history data with zeroes.
	xhistory.setlength(NUM_OLD_POSITIONS);
	yhistory.setlength(NUM_OLD_POSITIONS);
	thistory.setlength(NUM_OLD_POSITIONS);
	dhistory.setlength(NUM_OLD_POSITIONS);
	weights.setlength(NUM_OLD_POSITIONS);
	fmatrix.setlength(NUM_OLD_POSITIONS, MAX_DEGREE + 1);
	for (int i = 0; i < NUM_OLD_POSITIONS; i++){
		xhistory(i) = yhistory(i) = thistory(i) = 0;
		dhistory(i) = 1.0 / TIMESTEPS_PER_SECOND;
	}

	// Allocate space for the approximants.
	approxx.setlength(MAX_DEGREE + 1);
	approxy.setlength(MAX_DEGREE + 1);
	approxt.setlength(MAX_DEGREE + 1);
}

point predictable::future_position(double delta_time) const {
	double xx = 0, yy = 0;
	for (int i = MAX_DEGREE; i >= 0; i--){
		xx = xx * delta_time + approxx(i);
		yy = yy * delta_time + approxy(i);
	}
	return point(xx, yy);
}

double predictable::future_orientation(double delta_time) const {
	double tt = 0;
	for (int i = MAX_DEGREE; i >= 0; i--)
		tt = tt * delta_time + approxt(i);
	return tt;
}

point predictable::est_velocity() const {
	return point(approxx(1), approxy(1));
}

double predictable::est_avelocity() const {
	return approxt(1);
}

point predictable::est_acceleration() const {
	return point(approxx(2), approxy(2)) * 2;
}

double predictable::est_aacceleration() const {
	return approxt(2) * 2;
}

void predictable::add_prediction_datum(const point &pos, double orient, double delta_time) {
	// Move the new orientation value close to the previous value.
	while (std::fabs(orient - thistory(0)) > PI + 1e-9) {
		if (orient > thistory(0)) {
			orient -= 2 * PI;
		} else {
			orient += 2 * PI;
		}
	}

	// Add new position data to history list.
	for (int i = NUM_OLD_POSITIONS - 1; i; i--) {
		xhistory(i) = xhistory(i - 1);
		yhistory(i) = yhistory(i - 1);
		thistory(i) = thistory(i - 1);
		dhistory(i) = dhistory(i - 1);
	}
	xhistory(0) = pos.x;
	yhistory(0) = pos.y;
	thistory(0) = orient;
	dhistory(0) = delta_time;
	build_matrices();

	// If the orientation values are getting too big, reduce them to prevent floating point precision loss.
	while (std::fabs(thistory(0)) > 64 * PI) {
		double shift = thistory(0) > 0 ? -64 * PI : 64 * PI;
		for (int i = 0; i < NUM_OLD_POSITIONS; i++) {
			thistory(i) += shift;
		}
	}

	// Perform the regressions.
	buildgeneralleastsquares(xhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxx);
	buildgeneralleastsquares(yhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxy);
	buildgeneralleastsquares(thistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxt);
}

void predictable::clear_prediction(const point &pos, double orient) {
	// Fill the current position and orientation into all history cells. This
	// means that the object appears to be stationary.
	for (int i = 0; i < NUM_OLD_POSITIONS; ++i) {
		xhistory(i) = pos.x;
		yhistory(i) = pos.y;
		thistory(i) = orient;
		dhistory(i) = 1.0 / TIMESTEPS_PER_SECOND;
	}
	build_matrices();

	// Perform the regressions.
	buildgeneralleastsquares(xhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxx);
	buildgeneralleastsquares(yhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxy);
	buildgeneralleastsquares(thistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxt);
}

void predictable::build_matrices(){
	double dd = 0; // dd is always nonpositive
	for (int i = 0; i < NUM_OLD_POSITIONS; i++) {
		if (i) dd -= dhistory(i - 1);
		// Weights are 1 / (number of AI time steps from the present + 1).
		// Data further in the past will affect our predictions less.
		weights(i) = 1.0 / (1 - dd * TIMESTEPS_PER_SECOND);
		fmatrix(i, 0) = 1;
		for (unsigned int j = 1; j < MAX_DEGREE + 1; j++)
			fmatrix(i, j) = fmatrix(i, j - 1) * dd;
	}
}

