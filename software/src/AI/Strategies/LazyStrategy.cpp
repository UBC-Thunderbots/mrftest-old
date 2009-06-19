#include "AI/Strategies/LazyStrategy.h"
#include "AI/AITeam.h"
#include "datapool/World.h"

LazyStrategy::LazyStrategy(AITeam &team) : Strategy(team) {	
}

void LazyStrategy::update() {
	team.player(0)->plan(Plan::goalie);
	for (unsigned int i = 1; i < Team::SIZE; i++)
		team.player(i)->plan(Plan::stop);
}

