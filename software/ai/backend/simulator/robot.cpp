#include "ai/backend/simulator/robot.h"
#include "util/param.h"

using namespace AI::BE::Simulator;

namespace {
	DoubleParam linear_decay_constant("Robot Linear Decay Constant","Backend/Simulator",0.0,99.0,100.0);
	DoubleParam angular_decay_constant("Robot Angular Decay Constant","Backend/Simulator",0.0,99.0,100.0);
}

Robot::Robot(unsigned int pattern) : pattern_(pattern), xpred(1.3e-3, 2,linear_decay_constant), ypred(1.3e-3, 2,linear_decay_constant), tpred(Angle::of_radians(1.3e-3), Angle::of_radians(2),angular_decay_constant) {
} 

