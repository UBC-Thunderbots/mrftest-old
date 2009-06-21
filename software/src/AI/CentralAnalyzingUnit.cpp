#include <cassert>
#include <cmath>
#include <algorithm>
#include "AI/CentralAnalyzingUnit.h"
#include "datapool/World.h"

bool CentralAnalyzingUnit::checkVector(Vector2 rayOrigin, Vector2 rayEnd, PPlayer entity, unsigned int timeOffset, PPlayer passee, double radius) {
	
	if (radius == -1) radius = entity->radius();
	
	Vector2 rayDiff = rayEnd - rayOrigin;	
	
	Vector2 rayOrthagonal(rayDiff.angle()+90);
	
	rayOrthagonal *= radius;
	
	Vector2 rayOrigin1 = rayOrigin + rayOrthagonal;
	Vector2 rayOrigin2 = rayOrigin + rayOrthagonal;

	for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
		PPlayer p = World::get().player(i);
		if (p != entity && p != passee) {
			//initially only considering one frame ahead, should be possible to check certain amounts depending on how long the vector is
			Vector2 circlePos = p->futurePosition(timeOffset);
			double circleRadius = DEFAULT_MAX_ACC * 1 + p->radius() * 2;
			
			Vector2 circleDiff1 = circlePos - rayOrigin1;
			Vector2 circleDiff2 = circlePos - rayOrigin2;
			if (circleDiff1.length() <= circleRadius || circleDiff2.length() <= circleRadius) return true;
			
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
	
	// If the robot is not allowed close to the ball, consider its radius an obstacle:
	if (!entity->allowedInside()) {
		//initially only considering one frame ahead, should be possible to check certain amounts depending on how long the vector is
		Vector2 circlePos = World::get().ball().position();
		double circleRadius = World::get().field()->convertMmToCoord(800);
		
		Vector2 circleDiff1 = circlePos - rayOrigin1;
		Vector2 circleDiff2 = circlePos - rayOrigin2;
		if (circleDiff1.length() <= circleRadius || circleDiff2.length() <= circleRadius) return true;
		if (rayDiff.length() >= 1E-9 && circleDiff1.dot(rayDiff) > 0 && circleDiff2.dot(rayDiff) < 0 && (circleDiff1 - rayDiff*(circleDiff1.dot(rayDiff)/(rayDiff.length()*rayDiff.length()))).length() <= circleRadius) return true;

		if(entity->team().specialPossession()){
			Vector2 goalPos;
			PField field = World::get().field();
			if (entity->team().side()) goalPos = Vector2(field->east(), field->centerCircle().y);
			else goalPos = Vector2(field->west(), field->centerCircle().y);
			double goalRadius = World::get().field()->convertMmToCoord(800);	

			Vector2 goalDiff1 = goalPos - rayOrigin1;
			Vector2 goalDiff2 = goalPos - rayOrigin2;
			if (goalDiff1.length() <= goalRadius || goalDiff2.length() <= goalRadius) return true;
			if (rayDiff.length() >= 1E-9 && goalDiff1.dot(rayDiff) > 0 && goalDiff2.dot(rayDiff) < 0 && (goalDiff1 - rayDiff*(goalDiff1.dot(rayDiff)/(rayDiff.length()*rayDiff.length()))).length() <= goalRadius) return true;
		}
	}
	
	return false;
}

PPlayer CentralAnalyzingUnit::closestRobot(PPlayer robot, TEAM team, bool includeGoalie) {
	PPlayer closest;
	double closestDist = 1000000000;

	const std::vector<PPlayer> &robots =
		team == TEAM_ANY ? World::get().players() :
		team == TEAM_SAME ? robot->team().players() :
		robot->team().other().players();

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

Vector2 CentralAnalyzingUnit::lcIntersection(Vector2 start, Vector2 end, Vector2 centre, double radius) {
	// Normalize everything so the circle centres at the origin.
	start -= centre;
	end -= centre;
	// This algorithm came from Wolfram Mathworld.
	const double dx = end.x - start.x;
	const double dy = end.y - start.y;
	const double dr = std::sqrt(dx * dx + dy * dy);
	const double D = start.cross(end);
	const double discrim = radius * radius * dr * dr - D * D;
	if (discrim < 0)
		return end + centre;
	const double x1 = (D * dy + (dy < 0 ? -1 : 1) * dx * std::sqrt(discrim)) / (dr * dr);
	const double x2 = (D * dy - (dy < 0 ? -1 : 1) * dx * std::sqrt(discrim)) / (dr * dr);
	const double y1 = (-D * dx + std::fabs(dy) * std::sqrt(discrim)) / (dr * dr);
	const double y2 = (-D * dx - std::fabs(dy) * std::sqrt(discrim)) / (dr * dr);
	Vector2 p1(x1, y1);
	Vector2 p2(x2, y2);
	return ((p1 - start).length() < (p2 - start).length() ? p1 : p2) + centre;
}

