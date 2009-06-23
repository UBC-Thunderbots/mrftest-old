#include "AI/Strategies/CheaterStrategy.h"
#include "AI/CentralAnalyzingUnit.h"
#include "AI/AITeam.h"
#include "datapool/World.h"

#include <cstdlib>

CheaterStrategy::CheaterStrategy(AITeam &team) : Strategy(team), attackerWaiting(true), waitingCounter(0) {	
	for (unsigned int i = 0; i < Team::SIZE; i++)
		team.player(i)->plan(Plan::stop);
}

void CheaterStrategy::update() {
	World &w = World::get();
	const Field &field = w.field();

	bool dibs[] = {false, false, false, false, false};

	PPlayer goalieP;
	PPlayer attackerP;
	PPlayer defenderTopP;
	PPlayer defenderBottomP;
	PPlayer supporterP;

	// The first player is always goalie.
	goalieP = team.player(0);
	dibs[0] = true;

	// The closest player to the ball gets dibs to be attacker.
	double len = field.width() * 2.0;
	unsigned int closest = 0;
	for (unsigned int id = 1; id < Team::SIZE; id++) {
		if (!dibs[id]) {
			Vector2 dis = w.ball().position() - team.player(id)->position();
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
	}
	attackerP = team.player(closest);
	dibs[closest] = true;
	
	// hack to prevent players from passing back and forth.
	if (closest != oldAttacker) {
		attackerWaiting = true;
		oldAttacker = closest;
	}

	// The closest player to the upper corner gets to be defenderTop.
	len = field.width() * 2.0;
	closest = 0;
	for (unsigned int id = 1; id < Team::SIZE; id++) {
		if (!dibs[id]) {
			Vector2 corner;
			corner.y = field.north();
			if (team.side())
				corner.x = field.west();
			else
				corner.x = field.east();

			Vector2 dis = w.player(id)->position() - corner;
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
	}
	defenderTopP = team.player(closest);
	dibs[closest] = true;

	// The closest player to the lower corner gets to be defenderBottom.
	len = field.width() * 2.0;
	closest = 0;
	for (unsigned int id = 1; id < Team::SIZE; id++) {
		if (!dibs[id]) {
			Vector2 corner;
			corner.y = field.south();
			if (team.side())
				corner.x = field.west();
			else
				corner.x = field.east();

			Vector2 dis = w.player(id)->position() - corner;
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
	}
	defenderBottomP = team.player(closest);
	dibs[closest] = true;

	// The last player is supporter.
	unsigned int lastPlayer = 0;
	while (dibs[lastPlayer]) lastPlayer++;
	supporterP = team.player(lastPlayer);

	// Assign behaviors:
	goalie(goalieP);
	attacker(attackerP, supporterP);
	defenderTop(defenderTopP);
	defenderBottom(defenderBottomP);
	supporter(supporterP, attackerP);
}

void CheaterStrategy::goalie(PPlayer robot) {
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

void CheaterStrategy::attacker(PPlayer attacker, PPlayer supporter) {
	/*
	World &w = World::get();
	Vector2 len = attacker->position();
	if (team.side())
		len -= w.field()->eastGoal().penalty;
	else
		len -= w.field()->westGoal().penalty;

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
	}*/
	attacker->plan(Plan::chase);
}

void CheaterStrategy::supporter(PPlayer supporter, PPlayer attacker) {
	
	/*World &w = World::get();
	PField field = w.field();

	if (!attacker->hasBall()) {
		supporter->plan(Plan::chase);
	} else {
		Vector2 pos = w.ball()->position();

		Vector2 penalty; // The penalty point.
		if (!team.side())
			penalty = field.westGoal().penalty;
		else
			penalty = field.eastGoal().penalty;

		// Move towards the goal:
		supporter->plan(Plan::move);
		supporter->destination(penalty);
	}*/
	supporter->plan(Plan::chase);
}

void CheaterStrategy::defenderTop(PPlayer robot) {
	World &w = World::get();
	const Field &field = w.field();

	Vector2 pos = w.ball().position();
	double rad; // The size of the goal.
	if (team.side())
		rad = field.westGoal().south.y - field.westGoal().north.y;
	else
		rad = field.eastGoal().south.y - field.eastGoal().north.y;

	rad *= 0.5;

	Vector2 center; // The center position of the goal.
	if (team.side())
		center = Vector2(field.west(), field.height() / 2.0);
	else
		center = Vector2(field.east(), field.height() / 2.0);

	Vector2 vec = pos - center; // Vector between the ball and the goal.

	Vector2 target = w.ball().predictedVelocity();
	target *= (1.0 / target.length()); // get the unit vector.
	target *= vec.length();
	if (target.x != 0 && target.y != 0)
		target += pos;
	if (target.y < field.eastGoal().south.y && 
			target.y > field.eastGoal().north.y) {
		// If the ball is headed towards the goal, change the "center" of the goal to the ball's path.
		center.y = target.y;
	}

	// Adjust the center for top defender.
	center.y -= (robot->radius() * 2.0) + (((pos - center).length() / field.width()) *  field.height() / 10.0);

	// Find the destination point for the defender to move towards.
	double R = field.width() / 2.0;
	vec = pos - center;
	Vector2 des;
	if (vec.length() <= 4.0 * rad)
		des = center + vec * 4.0 * (rad / vec.length());
	else
		des = center + vec * 4.0 * (rad/ R);

	robot->plan(Plan::move);
	robot->destination(des);
}

void CheaterStrategy::defenderBottom(PPlayer robot) {
	World &w = World::get();
	const Field &field = w.field();
	Vector2 pos = w.ball().position();
	double rad; // The size of the goal.
	if (team.side())
		rad = field.westGoal().south.y - field.westGoal().north.y;
	else
		rad = field.eastGoal().south.y - field.eastGoal().north.y;

	rad *= 0.5;

	Vector2 center; // The center position of the goal.
	if (team.side())
		center = Vector2(field.west(), field.height() / 2.0);
	else
		center = Vector2(field.east(), field.height() / 2.0);

	Vector2 vec = pos - center; // Vector between the ball and the goal.

	Vector2 target = w.ball().predictedVelocity();
	target *= (1.0 / target.length()); // get the unit vector.
	target *= vec.length();
	if (target.x != 0 && target.y != 0)
		target += pos;
	if (target.y < field.eastGoal().south.y && 
			target.y > field.eastGoal().north.y) {
		// If the ball is headed towards the goal, change the "center" of the goal to the ball's path.
		center.y = target.y;
	}

	// Adjust the center for bottom defender.
	center.y += (robot->radius() * 2.0) + (((pos - center).length() / field.width()) * field.height() / 10.0);

	// Find the destination point for the defender to move towards.
	double R = field.width() / 2.0;
	vec = pos - center;
	Vector2 des;
	if(vec.length() <= 4.0 * rad)
		des = center + vec * 4.0 * (rad / vec.length());
	else
		des = center + vec * 4.0 * (rad / R);

	robot->plan(Plan::move);
	robot->destination(des);
}

