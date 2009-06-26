#include "AI/Strategies/ChaseStrategy.h"
#include "AI/AITeam.h"
#include "datapool/Plan.h"
#include "datapool/World.h"
#include "AI/RobotController.h"

#include <iostream>
using namespace std;

ChaseStrategy::ChaseStrategy(AITeam &team) : Strategy(team) {	
}

void ChaseStrategy::update() {	
	/*team.player(4)->plan(Plan::passer);
	team.player(4)->otherPlayer(team.player(1));	
	team.player(1)->receivingPass(true);*/
	for(int i = 1; i < 5; i++)
		team.player(i)->plan(Plan::chase);
}

