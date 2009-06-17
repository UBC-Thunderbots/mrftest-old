#include "AI/DecisionUnit.h"
#include "AI/Strategies/Strategy.h"
#include "AI/Strategies/OffensiveStrategy.h"
#include "AI/Strategies/DibsStrategy.h"
#include "AI/Strategies/DefenseStrategy.h"
#include "AI/Strategies/ChaseStrategy.h"
#include "AI/AITeam.h"
#include "datapool/World.h"

DecisionUnit::DecisionUnit(AITeam &team) : team(team) {
}

void DecisionUnit::update() {
	if (team.getCSU().strategy())
		return;
	PStrategy strat;
	if (&team == World::get().friendlyTeam().get())
		strat.reset(new ChaseStrategy(team));
	else
		strat.reset(new ChaseStrategy(team));
	team.getCSU().strategy(strat);
}

