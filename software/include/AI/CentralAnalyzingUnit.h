#ifndef TB_CENTRALANALYZINGUNIT_H
#define TB_CENTRALANALYZINGUNIT_H

#include <vector>

#include "datapool/Object.h"
#include "datapool/Player.h"
#include "datapool/Vector2.h"

namespace CentralAnalyzingUnit {
	const int FRAMES_PER_SECOND = 30;
	
	//Checks to see if there is an intersecting robot between two points.
	//Passe will be ignored.
	//Returns true if there is an intersecting robot, and false otherwise.
	bool checkVector(Vector2 rayOrigin, Vector2 rayEnd, PPlayer entity, unsigned int timeOffset, PPlayer passee = PPlayer());

	//Find the closest robot to the specified robot.
	enum TEAM {
		TEAM_ANY,
		TEAM_SAME,
		TEAM_OPPOSITE
	};
	PPlayer closestRobot(PPlayer robot, TEAM team, bool includeGoalie);
}

#endif

