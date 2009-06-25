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
	const double GOALIE_RADIUS = 400;

	// number of near-ball defenders
	const int NUM_NEAR_DEFENDERS = 2;

	// number of far-ball defenders
	const int NUM_FAR_DEFENDERS = 1;
}

DefenseStrategy::DefenseStrategy(AITeam &team) : Strategy(team) {
	for (unsigned int i = 0; i < Team::SIZE; i++)
		isUsed[i] = true;
}

void DefenseStrategy::init() {
	const Field &field = World::get().field();
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		PPlayer plr = team.player(i);
		const Vector2 &pos = plr->position();
		if (!field.isInfinity(pos.x) && !field.isInfinity(pos.y))
			isUsed[i] = false;
	}
}

void DefenseStrategy::farDefense() {
	const bool isWest = team.side();
	const World &w = World::get();
	const Field &field = w.field();
	const Goal &owngoal = isWest ? field.westGoal() : field.eastGoal();
	const Goal &enemygoal = isWest ? field.eastGoal() : field.westGoal();
	const double GoalieRadius = World::get().field().convertMmToCoord(GOALIE_RADIUS);

	// assign as goalie for now
	// do dynamic role later on
	PPlayer goalie = team.player(0);
	//PPlayer defender1 = team.player(1);
	//PPlayer defender2 = team.player(2);

	// goalpost north and south
	Vector2 goalpost1 = owngoal.north;
	Vector2 goalpost2 = owngoal.south;
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

	// find the ball position
	w.ball();	
	if((w.ball().position() - goalpost1).length() >
			(w.ball().position() - goalpost2).length()) {
		std::swap(goalpost2, goalpost1);
	}

	Vector2 goalieBlockPosition = calcBlockGoalie(goalpost1, goalpost2, w.ball().position(), GoalieRadius, radius);

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
	goalie->destination(goalieBlockPosition);

	Vector2 blockPosition[Team::SIZE];
	for (unsigned int i = 0; i < NUM_FAR_DEFENDERS; i++) {
		blockPosition[i] = calc_block_ray(goalpost1 - enemyPosition[distOrder[0]], goalpost2 - enemyPosition[distOrder[0]], radius) + enemyPosition[distOrder[0]];
	}

	assignDefenders(blockPosition, NUM_FAR_DEFENDERS);
}

void DefenseStrategy::nearDefense() {
	const bool isWest = team.side();
	const World &w = World::get();
	const Field &field = w.field();
	const Goal &owngoal = isWest ? field.westGoal() : field.eastGoal();
	const Goal &enemygoal = isWest ? field.eastGoal() : field.westGoal();
	const double GoalieRadius = World::get().field().convertMmToCoord(GOALIE_RADIUS);

	Vector2 goalpost1 = owngoal.north;
	Vector2 goalpost2 = owngoal.south;
	Vector2 goalpost = (goalpost1 + goalpost2) * 0.5;

	// rank the enemies based on distance
	// assign as goalie for now
	// do dynamic role later on
	PPlayer goalie = team.player(0);
	//PPlayer defender1 = team.player(1);
	//PPlayer defender2 = team.player(2);

	const double robotRadius = team.player(0)->radius();

	// rank the enemies based on distance
	unsigned int enemyDistOrder[Team::SIZE];
	double enemyDist[Team::SIZE];
	Vector2 enemyPosition[Team::SIZE];
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		enemyPosition[i] = team.other().player(i)->position();
		enemyDist[i] = (enemyPosition[i] - goalpost).length();
		enemyDistOrder[i] = i;
	}
	std::sort(enemyDistOrder, enemyDistOrder + sizeof(enemyDistOrder) / sizeof(*enemyDistOrder), SortByTable<double>(enemyDist));

	Vector2 blockPosition[Team::SIZE];
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		enemyPosition[i] = team.other().player(enemyDistOrder[i])->position();
		blockPosition[i] = calc_block_ray(goalpost1 - enemyPosition[i], goalpost2 - enemyPosition[i], robotRadius) + enemyPosition[i];
	}

	assignDefenders(blockPosition, NUM_NEAR_DEFENDERS);

	// the goalie always block the ball if possible..
	Vector2 ray = w.ball().position() - goalpost;

	// goalie always stay in some radius
	ray /= ray.length();
	ray *= GoalieRadius;
	goalie->plan(Plan::move);
	goalie->destination(ray + goalpost);
}

void DefenseStrategy::assignDefenders(const Vector2* blockPosition, int n) {
	// assign N defenders to defend M nearest enemies
	int assign[Team::SIZE], bestAssign[Team::SIZE];
	double bestAssignScore = 1e99;
	for (unsigned int i = 0; i < n; i++) assign[i] = i + 1;
	do {
		// calculate score of this assignment
		double score = 0;
		for (unsigned int i = 0; i < n; i++) {
			double dist = (blockPosition[i] - team.player(assign[i])->position()).length();
			score += dist * dist;
		}
		if(score >= bestAssignScore) continue;
		bestAssignScore = score;
		for (unsigned int i = 0; i < n; i++) {
			bestAssign[i] = assign[i];
		}
	} while(std::next_permutation(assign, assign + n));

	for (unsigned int i = 0; i < n; i++) {
		PPlayer defender = team.player(assign[i]);
		defender->plan(Plan::move);
		defender->destination(blockPosition[i]);
	}

	// remaining robots chase
	for (unsigned int i = n + 1; i < Team::SIZE; i++) {
		PPlayer defender = team.player(i);
		defender->plan(Plan::chase);
	}
}

void DefenseStrategy::defense() {
	const bool isWest = team.side();
	const World &w = World::get();
	const Field &field = w.field();
	const Goal &owngoal = isWest ? field.westGoal() : field.eastGoal();
	const Goal &enemygoal = isWest ? field.eastGoal() : field.westGoal();

	// check where the ball is
	// choose if need near or far defense
	if (abs(w.ball().position().x - owngoal.north.x) < abs(w.ball().position().x - enemygoal.north.x)) {
		nearDefense();
	} else {
		farDefense();
	}
}

void DefenseStrategy::update() {
	init();
	defense();
}

