#include "AI/Strategies/ShootingChallengeStrategy.h"
#include "AI/AITeam.h"
#include "datapool/Plan.h"
#include "datapool/World.h"
#include "AI/RobotController.h"

#include <iostream>
using namespace std;

ShootingChallengeStrategy::ShootingChallengeStrategy(AITeam &team) : Strategy(team) {	
}

void ShootingChallengeStrategy::update() {
	for(int i = 0; i < 2; i++) // robots zero and one chase.
		team.player(i)->plan(Plan::chase);
}

