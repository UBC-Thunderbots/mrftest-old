#ifndef AI_CENTRALANALYZINGUNIT_H
#define AI_CENTRALANALYZINGUNIT_H

#include <vector>

#include "datapool/Object.h"
#include "datapool/Player.h"
#include "datapool/Vector2.h"

namespace CentralAnalyzingUnit {
	const int FRAMES_PER_SECOND = 14;
	
	//Checks to see if there is an intersecting robot between two points.
	//Passe will be ignored.
	//Returns true if there is an intersecting robot, and false otherwise.
	bool checkVector(Vector2 rayOrigin, Vector2 rayEnd, PPlayer entity, unsigned int timeOffset, PPlayer passee = PPlayer(), double radius = -1);

	//Find the closest robot to the specified robot.
	enum TEAM {
		TEAM_ANY,
		TEAM_SAME,
		TEAM_OPPOSITE
	};
	PPlayer closestRobot(PPlayer robot, TEAM team, bool includeGoalie);

	// Finds the point of intersection between a line segment and a circle.
	// If the segment [start,end] does not intersect the circle, returns end.
	// If it does intersect, returns the closer of the two intersection points to start.
	Vector2 lcIntersection(Vector2 start, Vector2 end, Vector2 centre, double radius);
}

#endif

