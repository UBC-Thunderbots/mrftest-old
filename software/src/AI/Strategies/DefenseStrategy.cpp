#include "AI/AITeam.h"
#include "AI/Strategies/DefenseStrategy.h"
#include "AI/RobotAlgorithmUtils.h"
#include "AI/RobotController.h"
#include "datapool/World.h"

#include <iostream>
#include <algorithm>
#include <cstring>
#include <climits>
#include <cassert>

namespace {
	//
	// A comparator that sorts by values in a table.
	//
	template<typename T>
	class SortByTable {
	public:
		SortByTable(const T *tbl) : tbl(tbl) {
		}

		bool operator()(unsigned int x, unsigned int y) {
			return tbl[x] < tbl[y];
		}

	private:
		const T *tbl;
	};

	// distance from goalpost that goalie should be
	const double GoalieRadius = 20;
}

DefenseStrategy::DefenseStrategy(AITeam &team) : Strategy(team) {
	for (unsigned int i = 0; i < Team::SIZE; i++)
		isUsed[i] = true;
}

void DefenseStrategy::init() {
	PField fld = World::get().field();
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		PPlayer plr = team.player(i);
		const Vector2 &pos = plr->position();
		if (!fld->isInfinity(pos.x) && !fld->isInfinity(pos.y))
			isUsed[i] = false;
	}
}

// A = friendly team goal north
// B = friendly team goal south
// C = ball 
// Computes G, goalie position 
//          D, second defender position if needed to cover the goal from direct score of the ball current location.
void DefenseStrategy::defense() {
	bool isWest = team.side();

	const World &w = World::get();
	const PField field = w.field();
	PGoal goal = isWest ? field->westGoal() : field->eastGoal();

	// assign as goalie for now
	// do dynamic role later on
	PPlayer goalie = team.player(0);
	//PPlayer defender1 = team.player(1);
	//PPlayer defender2 = team.player(2);

	// goalpost north and south
	Vector2 goalpost1 = goal->north;
	Vector2 goalpost2 = goal->south;
	Vector2 goalpost = (goalpost1 + goalpost2) * 0.5;

	// rank the enemies based on distance
	unsigned int distOrder[Team::SIZE];
	double dist[Team::SIZE];
	Vector2 enemyPosition[Team::SIZE];
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		enemyPosition[i] = team.other().player(i)->position();
		dist[i] = (enemyPosition[i] - goalpost).length();
		distOrder[i] = i;
	}
	std::sort(distOrder, distOrder + sizeof(distOrder) / sizeof(*distOrder), SortByTable<double>(dist));

	// radius of a robot
	const double radius = team.player(0)->radius();

	/*
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
			*/
	
	// find the ball position
	w.ball();	
	if((w.ball()->position() - goalpost1).length() >
		(w.ball()->position() - goalpost2).length()) {
		std::swap(goalpost2, goalpost1);
	}

	Vector2 blockPosition = calcBlockGoalie(goalpost1, goalpost2, w.ball()->position(), GoalieRadius, radius);

	//Vector2 blockPosition1 = defenderBlocksGoalPost(goalpost1, goalpost2, w.ball()->position(), blockPosition, radius);

	// rank the defenders based on distance
	unsigned int defOrder[Team::SIZE];
	double defDist[Team::SIZE];
	Vector2 playerPosition[Team::SIZE];
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		//playerPosition[i] = team.player(i)->position();
		//defDist[i] = (playerPosition[i] - blockPosition1).length();
		defOrder[i] = i;
	}
	//std::sort(defOrder + 1, defOrder + sizeof(defOrder) / sizeof(*defOrder), SortByTable<double>(defDist));

	// move the defender
	goalie->plan(Plan::move);
	goalie->destination(blockPosition);

	PPlayer defender1 = team.player(defOrder[1]);
	defender1->plan(Plan::move);
	//defender1->destination(blockPosition1);

	PPlayer defender2 = team.player(defOrder[2]);

	//defender1->destination(enemyPosition[distOrder[0]]);
	//defender2->destination(enemyPosition[distOrder[1]]);

	//Vector2 blockPosition1 = defenderBlocksGoalPost(goalpost1, goalpost2, w.ball()->position(), blockPosition, radius);
	//Vector2 blockPosition2 = goalpost2;

	Vector2 blockPosition1 = calc_block_ray(goalpost1 - enemyPosition[distOrder[0]], goalpost2 - enemyPosition[distOrder[0]], radius) + enemyPosition[distOrder[0]];
	Vector2 blockPosition2 = calc_block_ray(goalpost1 - enemyPosition[distOrder[1]], goalpost2 - enemyPosition[distOrder[1]], radius) + enemyPosition[distOrder[1]];

	if ((blockPosition1 - defender1->position()).length() < (blockPosition1 - defender2->position()).length()) {
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

