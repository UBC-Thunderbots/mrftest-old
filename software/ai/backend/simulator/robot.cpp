#include "ai/backend/simulator/robot.h"
#include "util/param.h"

using namespace AI::BE::Simulator;

namespace {
	DoubleParam LINEAR_DECAY_CONSTANT("Robot Linear Decay Constant","Backend/Simulator",0.0,99.0,100.0);
	DoubleParam ANGULAR_DECAY_CONSTANT("Robot Angular Decay Constant","Backend/Simulator",0.0,99.0,100.0);
}

Robot::Robot(unsigned int pattern) : pattern_(pattern), xpred(1.3e-3, 2,LINEAR_DECAY_CONSTANT), ypred(1.3e-3, 2,LINEAR_DECAY_CONSTANT), tpred(Angle::of_radians(1.3e-3), Angle::of_radians(2),ANGULAR_DECAY_CONSTANT) {
} 

