#include "ai/navigator/world.h"

using AI::Nav::W::Robot;
using AI::Nav::W::Player;

const double Robot::MAX_RADIUS = 0.09;
const double Player::MAX_LINEAR_VELOCITY = 2;
const double Player::MAX_LINEAR_ACCELERATION = 3.5;
const Angle Player::MAX_ANGULAR_VELOCITY = Angle::of_radians(38);
const Angle Player::MAX_ANGULAR_ACCELERATION = Angle::of_radians(77.7);

