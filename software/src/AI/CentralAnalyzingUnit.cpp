#include <cassert>
#include <algorithm>
#include "AI/CentralAnalyzingUnit.h"
#include "datapool/World.h"

bool CentralAnalyzingUnit::checkVector(Vector2 rayOrigin, Vector2 rayEnd, PPlayer entity, unsigned int timeOffset, PPlayer passee) {
	
	Vector2 rayDiff = rayEnd - rayOrigin;	
	
	Vector2 rayOrthagonal(rayDiff.angle()+90);
	
	Vector2 rayOrigin1 = rayOrigin + rayOrthagonal;
	Vector2 rayOrigin2 = rayOrigin + rayOrthagonal;
	
	rayOrthagonal *= entity->radius();

	for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
		PPlayer p = World::get().player(i);
		if (p != entity && p != passee) {
			//initially only considering one frame ahead, should be possible to check certain amounts depending on how long the vector is
			Vector2 circlePos = p->futurePosition(timeOffset);
			double circleRadius = DEFAULT_MAX_ACC * 1 + p->radius() * 2;
			
			Vector2 circleDiff1 = circlePos - rayOrigin1;
			Vector2 circleDiff2 = circlePos - rayOrigin2;
			
			double u = circleDiff1.dot(rayDiff) / rayDiff.dot(rayDiff);
	
			if(u < 1 && u >= 0 && (rayOrigin1 + (rayDiff * u) - circlePos).length() < circleRadius) {
				return true;
			}
			
			u = circleDiff2.dot(rayDiff) / rayDiff.dot(rayDiff);
			
			if(u < 1 && u >= 0 && (rayOrigin2 + (rayDiff * u) - circlePos).length() < circleRadius) {
				return true;
			}
		}
	}
	return false;
}

PPlayer CentralAnalyzingUnit::closestRobot(PPlayer robot, TEAM team, bool includeGoalie) {
	PPlayer closest;
	double closestDist = 1000000000;

	const std::vector<PPlayer> &robots =
		team == TEAM_ANY ? World::get().players() :
		team == TEAM_SAME ? robot->team()->players() :
		robot->team()->other()->players();

	for (unsigned int i = 0; i < robots.size(); i++) {
		if (robots[i] != robot && (includeGoalie || (i != 0 && i != Team::SIZE))) {
			double dist = (robot->position() - robots[i]->position()).length();
			if (dist < closestDist) {
				closest = robots[i];
				closestDist = dist;
			}
		}
	}
	return closest;
}

