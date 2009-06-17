#include "AI/Strategies/DefenseStrategy.h"
#include "AI/AITeam.h"
#include "datapool/World.h"
#include "AI/RobotController.h"
#include "AI/RobotAlgorithmUtils.h"
#include <cstring>

#include <iostream>

DefenseStrategy::DefenseStrategy(AITeam &team) : Strategy(team) {
	World & w = World::get();
	field = w.field();
	//pTeam = PTeam(team);
	pOther = team.other();
	pBall = w.ball();
	for (unsigned int i = 0; i < Team::SIZE; i++) isUsed[i] = true;
}

DefenseStrategy::~DefenseStrategy() {
}

void DefenseStrategy::init() {
	World & w = World::get();
	for (int i = w.getNumActiveFriendlyPlayers() - 1; i >= 0; --i) isUsed[i] = false;
}

// A = friendly team goal north
// B = friendly team goal south
// C = ball 
// Computes G, goalie position 
//          D, second defender position if needed to cover the goal from direct score of the ball current location.
void DefenseStrategy::defense() {
	bool isWest = team.side();
	
	const World &w = World::get();
	const PField& field = w.field();
	PGoal goal;

	// assign as goalie for now
	// do dynamic role later on
	PPlayer goalie = team.player(0);
	PPlayer defender1 = team.player(1);
	PPlayer defender2 = team.player(2);
	
	if (isWest) goal = field->westGoal();
	else goal = field->eastGoal();
	
	// goalpost north and south
	const Vector2& goalpost1 = goal->north;
	const Vector2& goalpost2 = goal->south;
	Vector2 goalpost = (goalpost1 + goalpost2) * 0.5;

	// ball position
	const Vector2& ball = w.ball()->position();

	// find the enemy player with the ball, if exist

	// rank the enemies based on distance
	int distOrder[Team::SIZE];
	double dist[Team::SIZE];
	Vector2 enemyPosition[Team::SIZE];
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		enemyPosition[i] = pOther->player(i)->position();
		dist[i] = length(enemyPosition[i] - goalpost);
		distOrder[i] = i;
	}

	// do stupid bubble sort
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		for (unsigned int j = i + 1; j < Team::SIZE; j++) {
			if(dist[distOrder[i]] > dist[distOrder[j]]) {
				std::swap(distOrder[i], distOrder[j]);
			}
		}
	}

	// the first 2 enemies will shoot out rays to the goal post
	// find the region intersected by the 2 rays

	//std::cout << " position= " << enemyPosition[distOrder[0]].x
	//	<< " "
	//		<< enemyPosition[distOrder[0]].y << std::endl;

	// radius of a robot
	const double radius = team.player(0)->radius();

	// intersect is where the 2 rays meet
	Vector2 intersect;
	Vector2 blockPosition1, blockPosition2;
	if(seg_crosses_seg(goalpost2, enemyPosition[distOrder[0]],
			goalpost1, enemyPosition[distOrder[1]])) {
		// the nearer player is at the top
		intersect = line_intersect(goalpost2, enemyPosition[distOrder[0]],
			goalpost1, enemyPosition[distOrder[1]]);
		// std::cout << " near at top " << std::endl;

		blockPosition1 = calc_block_ray(
				goalpost1 - enemyPosition[distOrder[0]],
				goalpost - enemyPosition[distOrder[0]], radius)
			+ enemyPosition[distOrder[0]];

		blockPosition2 = calc_block_ray(
				goalpost2 - enemyPosition[distOrder[1]],
				goalpost - enemyPosition[distOrder[1]], radius)
			+ enemyPosition[distOrder[1]];
	} else {
		// the nearer player is at the bottom
		intersect = line_intersect(goalpost1, enemyPosition[distOrder[0]],
				goalpost2, enemyPosition[distOrder[1]]);
		// std::cout << " near at bot " << std::endl;

		blockPosition1 = calc_block_ray(
				goalpost1 - enemyPosition[distOrder[1]],
				goalpost - enemyPosition[distOrder[1]], radius)
			+ enemyPosition[distOrder[1]];

		blockPosition2 = calc_block_ray(
				goalpost2 - enemyPosition[distOrder[0]],
				goalpost - enemyPosition[distOrder[0]], radius)
			+ enemyPosition[distOrder[0]];
	}

	// find where the defender should defend
	Vector2 blockPosition = calc_block_ray(
			goalpost1 - intersect,
			goalpost2 - intersect, radius) + intersect;

	// move the defender
	goalie->plan(Plan::move);
	goalie->destination(blockPosition);
	if(length(blockPosition1 - defender1->position()) <
			length(blockPosition1 - defender2->position())) {
		defender1->plan(Plan::move);
		defender1->destination(blockPosition1);
		defender2->plan(Plan::move);
		defender2->destination(blockPosition2);
	} else {
		defender2->plan(Plan::move);
		defender2->destination(blockPosition1);
		defender1->plan(Plan::move);
		defender1->destination(blockPosition2);
	}
}

void DefenseStrategy::update() {
	init();

	defense();
}

