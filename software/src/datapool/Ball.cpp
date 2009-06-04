#include "datapool/Ball.h"

Ball::Ball() {
}

PBall Ball::create() {
	PBall ball(new Ball);
	return ball;
}

