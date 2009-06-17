#include "AI/Strategies/DefenseStrategy.h"
#include "AI/AITeam.h"
#include "datapool/World.h"
#include "AI/RobotController.h"
#include <cstring>


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
	/*
	bool isWest = team->side();
	
	PGoal pGoal;
	
	if (isWest) pGoal = field.westGoal();
	else pGoal = field.eastGoal();
	
	Vector2 A = pGoal.north;
	Vector2 B = pGoal.south;
	Vector2 C = w.pBall().position();
	*/
	
}

void DefenseStrategy::update() {
	init();
		
	defense();
}

