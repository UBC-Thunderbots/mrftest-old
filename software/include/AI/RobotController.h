#ifndef TB_ROBOTCONTROLLER_H
#define TB_ROBOTCONTROLLER_H

#include "datapool/Player.h"
#include "datapool/Vector2.h"

namespace RobotController {
	void setSimulation(bool simulation);
	void sendCommand(PPlayer robot, Vector2 acc, double rotate, unsigned char dribble, unsigned char kick);
}

#endif

