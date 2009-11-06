#include "leastsquares/leastsquares.h"
#include "world/predictable.h"

namespace {
	const int MAX_DEGREE = 3;
	const int NUM_OLD_POSITIONS = 6;
}

predictable::predictable() {
	// Fill history data with zeroes.
	xhistory.setlength(NUM_OLD_POSITIONS);
	yhistory.setlength(NUM_OLD_POSITIONS);
	for (int i = 0; i < NUM_OLD_POSITIONS; i++)
		xhistory(i) = yhistory(i) = 0;

	// For now, weights are 1/(i+1).
	weights.setlength(NUM_OLD_POSITIONS);
	for (int i = 0; i < NUM_OLD_POSITIONS; i++)
		weights(i) = 1.0 / (i + 1);

	// For now, F matrix is static.
	fmatrix.setlength(NUM_OLD_POSITIONS, MAX_DEGREE + 1);
	for (int i = 0; i < NUM_OLD_POSITIONS; i++) {
		fmatrix(i, 0) = 1;
		for (unsigned int j = 1; j < MAX_DEGREE + 1; j++)
			fmatrix(i, j) = fmatrix(i, j - 1) * -i;
	}

	// Allocate space for the approximants.
	approxx.setlength(MAX_DEGREE + 1);
	approxy.setlength(MAX_DEGREE + 1);
}

point predictable::est_velocity() const {
	return point(approxx(1), approxy(1));
}

void predictable::add_prediction_datum(const point &pos) {
	// Add new position data to history list.
	for (int i = NUM_OLD_POSITIONS - 1; i; i--) {
		xhistory(i) = xhistory(i - 1);
		yhistory(i) = yhistory(i - 1);
	}
	xhistory(0) = pos.x;
	yhistory(0) = pos.y;

	// Perform the regressions.
	buildgeneralleastsquares(xhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxx);
	buildgeneralleastsquares(yhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxy);
}

