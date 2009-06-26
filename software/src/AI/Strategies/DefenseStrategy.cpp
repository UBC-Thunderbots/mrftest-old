#include "AI/Strategies/DefenseStrategy.h"
#include "AI/CentralAnalyzingUnit.h"
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
			return tbl[x] > tbl[y];
		}

	private:
		const T *tbl;
	};

	// distance from goalpost that goalie should be
	const double GOALIE_RADIUS = 400;
	const double DEFENSE_RADIUS = 600;

	// number of near-ball defenders
	const int NUM_NEAR_DEFENDERS = 2;

	// number of far-ball defenders
	const int NUM_FAR_DEFENDERS = 2;
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

	// goalpost north and south
	Vector2 goalpost1 = owngoal.north;
	Vector2 goalpost2 = owngoal.south;
	Vector2 goalpost = (goalpost1 + goalpost2) * 0.5;

	// rank the enemies based on threat
	// threat = weight / distance
	unsigned int threatOrder[Team::SIZE];
	double threat[Team::SIZE];
	Vector2 enemyPosition[Team::SIZE];
	double enemyClosestToBallDistance = 1e99;
	unsigned int enemyClosestToBall = 0;
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		enemyPosition[i] = team.other().player(i)->position();
		threat[i] = 1.0 / (0.0001 + (enemyPosition[i] - goalpost).length());
		threatOrder[i] = i;
		double distanceToBall = (enemyPosition[i] - w.ball().position()).length();
		if (distanceToBall < enemyClosestToBallDistance) {
			enemyClosestToBall = i;
			enemyClosestToBallDistance = distanceToBall;
		}
	}
	threat[enemyClosestToBall] *= 1e20;
	std::sort(threatOrder, threatOrder + sizeof(threatOrder) / sizeof(*threatOrder), SortByTable<double>(threat));

	// radius of a robot
	const double radius = team.player(0)->radius();

	// find the ball position
	// and check whether to use top or bottom defense position
	w.ball();	
	if((w.ball().position() - goalpost1).length() > (w.ball().position() - goalpost2).length()) {
		std::swap(goalpost2, goalpost1);
	}

	Vector2 goalieBlockPosition = calcBlockGoalie(goalpost1, goalpost2, enemyPosition[threatOrder[0]], GoalieRadius, radius * 2);

	// move the goalie
	goalie->plan(Plan::move);
	goalie->destination(goalieBlockPosition);

	Vector2 blockPosition[Team::SIZE];
	for (unsigned int i = 0; i < NUM_FAR_DEFENDERS; i++) {
		blockPosition[i] = calc_block_ray(goalpost1 - enemyPosition[threatOrder[i]], goalpost2 - enemyPosition[threatOrder[i]], radius) + enemyPosition[threatOrder[i]];
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

	const Vector2 &goalpost1 = owngoal.north;
	const Vector2 &goalpost2 = owngoal.south;
	Vector2 goalpost = (goalpost1 + goalpost2) * 0.5;

	// rank the enemies based on distance
	// assign as goalie for now
	// do dynamic role later on
	PPlayer goalie = team.player(0);

	const double robotRadius = team.player(0)->radius();

	// rank the enemies based on threat
	// threat = weight / distance
	unsigned int threatOrder[Team::SIZE];
	double threat[Team::SIZE];
	Vector2 enemyPosition[Team::SIZE];
	double enemyClosestToBallDistance = 1e99;
	unsigned int enemyClosestToBall = 0;
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		enemyPosition[i] = team.other().player(i)->position();
		threat[i] = 1.0 / (0.0001 + (enemyPosition[i] - goalpost).length());
		threatOrder[i] = i;
		double distanceToBall = (enemyPosition[i] - w.ball().position()).length();
		if (distanceToBall < enemyClosestToBallDistance) {
			enemyClosestToBall = i;
			enemyClosestToBallDistance = distanceToBall;
		}
	}
	threat[enemyClosestToBall] *= 1e20;
	std::sort(threatOrder, threatOrder + sizeof(threatOrder) / sizeof(*threatOrder), SortByTable<double>(threat));

	Vector2 blockPosition[Team::SIZE];
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		enemyPosition[i] = team.other().player(threatOrder[i])->position();
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

void DefenseStrategy::assignDefenders(Vector2* blockPosition, int n) {
	const bool isWest = team.side();
	const World &w = World::get();
	const Field &field = w.field();
	const Goal &owngoal = isWest ? field.westGoal() : field.eastGoal();
	const Goal &enemygoal = isWest ? field.eastGoal() : field.westGoal();
	const Vector2 &goalpost1 = owngoal.north;
	const Vector2 &goalpost2 = owngoal.south;
	Vector2 goalpost = (goalpost1 + goalpost2) * 0.5;

	const double DefenseRadius = World::get().field().convertMmToCoord(DEFENSE_RADIUS);
	
	// note we have to enforce the fact that defenders cannot stay too long in the defense area
	for (unsigned int i = 0; i < n; i++) {
		Vector2 ray = blockPosition[i] - goalpost;
		if (ray.length() < DefenseRadius) {
			ray /= ray.length();
			ray *= DefenseRadius;
		}
		blockPosition[i] = ray + goalpost;
	}

	// assign N defenders to defend M nearest enemies
	unsigned int assign[Team::SIZE], bestAssign[Team::SIZE];

	// closest to the ball will chase..
	double closestToBallDist = 1e99;
	unsigned int closestToBall = UINT_MAX;
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		//if (team.player(i)->hasBall()) hasBall = i;
		double dist = (w.ball().position() - team.player(i)->position()).length();
		if (dist < closestToBallDist) {
			closestToBallDist = dist;
			closestToBall = i;
		}
	}

	unsigned int j = 1;
	for (unsigned int i = 0; i < n; i++) {
		if (i + j == closestToBall) j++;
		assign[i] = i + j;
	}

	double bestAssignScore = 1e99;
	do {
		// calculate score of this assignment
		double score = 1e99;
		for (unsigned int i = 0; i < n; i++) {
			double dist = (blockPosition[i] - team.player(assign[i])->position()).length();
			score += dist * dist;
		}
		if(score > bestAssignScore) continue;
		bestAssignScore = score;
		for (unsigned int i = 0; i < n; i++) {
			bestAssign[i] = assign[i];
		}
	} while(std::next_permutation(assign, assign + n));

	bool used[Team::SIZE];
	std::fill(used, used + Team::SIZE, false);

	for (unsigned int i = 0; i < n; i++) {
		PPlayer defender = team.player(bestAssign[i]);
		defender->plan(Plan::move);
		defender->destination(blockPosition[i]);
		used[bestAssign[i]] = true;
	}
	used[closestToBall] = true;

	// choose 2 robots to be attacker/supporter
	unsigned int isupport = UINT_MAX;
	unsigned int iattack = UINT_MAX;

	// remaining robots chase
	for (unsigned int i = 1; i < Team::SIZE; i++) {
		if (used[i]) continue;
		PPlayer defender = team.player(i);
		defender->plan(Plan::chase);

		if (iattack == UINT_MAX) iattack = i;
		else if (isupport = UINT_MAX) isupport = i;
	}

	// we have 2 spare robots, use to attack/support
	if (isupport != UINT_MAX) {
		//attacker(team.player(iattack), team.player(isupport));
		//supporter(team.player(isupport), team.player(iattack));
	}

	// the closest to ball also chase
	team.player(closestToBall)->plan(Plan::chase);
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
		nearDefense();
	}

	/*
	// in the event that ANY defender has the ball, don't use it for defence
	unsigned int ballholder = UINT_MAX;
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		if (team.player(i)->hasBall()) {
			ballholder = i;
		}
	}

	// the player having the ball overrides everything
	if (ballholder != UINT_MAX) {
		team.player(ballholder)->plan(Plan::chase);
	}
	*/

	goalie(team.player(0));
}

void DefenseStrategy::update() {
	init();
	defense();
}

void DefenseStrategy::goalie(PPlayer robot) {
	if (robot->hasBall()) {
		PPlayer passee = CentralAnalyzingUnit::closestRobot(robot, CentralAnalyzingUnit::TEAM_SAME, false);

		for (unsigned int i = 0; i < Team::SIZE; i++)
			if (team.player(i)->receivingPass())
				passee = team.player(i);

		robot->plan(Plan::passer);
		robot->otherPlayer(passee);
		passee->receivingPass(true);
	} else {
		robot->plan(Plan::goalie);
	}
}

void DefenseStrategy::attacker(PPlayer attacker, PPlayer supporter) {
	/*
	World &w = World::get();
	Vector2 len = attacker->position();
	if (team.side())
		len -= w.field().eastGoal().penalty;
	else
		len -= w.field().westGoal().penalty;

	if (attacker->hasBall() && len.length() < 100.0) {
		attacker->plan(Plan::shoot);
	} else if (attacker->hasBall() && attackerWaiting) {
		attacker->plan(Plan::move);
		attacker->destination(attacker->position());
		waitingCounter++;
		if (waitingCounter == 30){
			waitingCounter = 0;
			attackerWaiting = false;
		}
	} else if (attacker->hasBall()) {
		attacker->plan(Plan::passer);
		attacker->otherPlayer(supporter);
		supporter->receivingPass(true);	
	} else {
		attacker->plan(Plan::shoot);
		waitingCounter = 0;
		attackerWaiting = true;
	}
	*/
}

void DefenseStrategy::supporter(PPlayer supporter, PPlayer attacker) {
	/*
	World &w = World::get();
	const Field &field = w.field();

	if (!attacker->hasBall()) {
		supporter->plan(Plan::chase);
	} else {
		Vector2 pos = w.ball().position();

		Vector2 penalty; // The penalty point.
		if (!team.side())
			penalty = field.westGoal().penalty;
		else
			penalty = field.eastGoal().penalty;

		// Move towards the goal:
		supporter->plan(Plan::move);
		supporter->destination(penalty);
	}
	*/
}

