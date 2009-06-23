#include "datapool/PredictableObject.h"
#include "datapool/World.h"
#include "AI/CentralAnalyzingUnit.h"
#include "../src/leastsquares/leastsquares.h"

#define MAX_DEGREE 3
#define NUM_OLD_POSITIONS 6
#define VELOCITY_THRESHOLD 20000

PredictableObject::PredictableObject() : predVel(), predAcc() {
	// Fill history data with zeroes.
	xhistory.setlength(NUM_OLD_POSITIONS);
	yhistory.setlength(NUM_OLD_POSITIONS);
	for (unsigned int i = 0; i < NUM_OLD_POSITIONS; i++)
		xhistory(i) = yhistory(i) = 0;

	// For now, weights are 1/(i+1).
	weights.setlength(NUM_OLD_POSITIONS);
	for (unsigned int i = 0; i < NUM_OLD_POSITIONS; i++)
		weights(i) = 1.0/(i+1);

	// For now, F matrix is static.
	fmatrix.setlength(NUM_OLD_POSITIONS, MAX_DEGREE + 1);
	for (int i = 0; i < NUM_OLD_POSITIONS; i++) {
		fmatrix(i, 0) = 1;
		for (unsigned int j = 1; j < MAX_DEGREE + 1; j++)
			fmatrix(i, j) = fmatrix(i, j - 1) * (-i);
	}

	// Allocate space for the approximants.
	approxx.setlength(MAX_DEGREE + 1);
	approxy.setlength(MAX_DEGREE + 1);
}

void PredictableObject::update() {
	if ((Object::position() - Vector2(xhistory(0), yhistory(0))).length() > World::get().field()->convertMmToCoord(VELOCITY_THRESHOLD) / CentralAnalyzingUnit::FRAMES_PER_SECOND) {
		for (unsigned int i = 0; i < NUM_OLD_POSITIONS; i++) {
			xhistory(i) = Object::position().x;
			yhistory(i) = Object::position().y;
		}
	}
	// Add new position data to history list.
	else {
		for (unsigned int i = NUM_OLD_POSITIONS - 1; i; i--) {
			xhistory(i) = xhistory(i - 1);
			yhistory(i) = yhistory(i - 1);
		}
		xhistory(0) = Object::position().x;
		yhistory(0) = Object::position().y;
	}

	// Perform the regressions.
	buildgeneralleastsquares(xhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxx);
	buildgeneralleastsquares(yhistory, weights, fmatrix, NUM_OLD_POSITIONS, MAX_DEGREE + 1, approxy);

	// TODO: Cedric: set predVel and predAcc vectors!
	// predVel.x = ?
	// predVel.y = ?
	// predAcc.x = ?
	// predAcc.y = ?
}

Vector2 PredictableObject::futurePosition(unsigned int timeOffset) {
	Vector2 result(0, 0);
	for (unsigned int i = MAX_DEGREE; i; i--) {
		result.x = result.x * timeOffset + approxx(i - 1);
		result.y = result.y * timeOffset + approxy(i - 1);
	}
	return result;
}

