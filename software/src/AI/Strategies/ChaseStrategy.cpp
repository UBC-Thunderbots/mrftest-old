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
	team.player(0)->plan(Plan::chase);
	for(int i = 1; i < 5; i++)
		team.player(i)->plan(Plan::chase);
}

