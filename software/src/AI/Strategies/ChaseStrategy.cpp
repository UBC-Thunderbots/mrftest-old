#include "AI/Strategies/ChaseStrategy.h"
#include "AI/AITeam.h"
#include "datapool/World.h"

ChaseStrategy::ChaseStrategy(AITeam &team) : Strategy(team) {	
}

ChaseStrategy::~ChaseStrategy() {
}

void ChaseStrategy::update() {
	for (unsigned int i = 0; i < Team::SIZE; i++)
		team.player(i)->plan(Plan::chase);
		
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		if (team.player(i)->hasBall()) {
			team.player(i)->plan(Plan::stop);
		}
	}
}

