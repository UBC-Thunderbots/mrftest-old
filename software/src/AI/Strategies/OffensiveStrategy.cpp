#include "AI/Strategies/OffensiveStrategy.h"
#include "AI/CentralAnalyzingUnit.h"
#include "AI/AITeam.h"
#include "datapool/World.h"

OffensiveStrategy::OffensiveStrategy(AITeam &team) : Strategy(team) {	
	PField field = World::get().field();

	if (team.side())
		defenseRange = field->west() + field->width() / 5.0;
	else
		defenseRange = field->east() - field->width() / 5.0;

	team.player(2)->plan(Plan::move);
	team.player(2)->destination(Vector2(defenseRange, 270));
	team.player(3)->plan(Plan::shoot);
	team.player(4)->plan(Plan::shoot);
	team.player(1)->plan(Plan::shoot);
	team.player(0)->plan(Plan::goalie);
}

OffensiveStrategy::~OffensiveStrategy() {
}

void OffensiveStrategy::update() {
	team.player(2)->plan(Plan::move);
	if (team.player(2)->destination().x != defenseRange)
		team.player(2)->destination(Vector2(defenseRange, 270));
	team.player(3)->plan(Plan::shoot);
	team.player(4)->plan(Plan::shoot);
	team.player(1)->plan(Plan::shoot);
	team.player(0)->plan(Plan::goalie);
		
	Vector2 pos = team.player(2)->position();
	if (pos.y - 190 < 3 && pos.y - 190 > -3)
		team.player(2)->destination(Vector2(defenseRange, 270));
		
	pos = team.player(2)->position();
	if (pos.y - 270 < 3 && pos.y - 270 > -3)
		team.player(2)->destination(Vector2(defenseRange, 190));
		
	if (team.player(2)->hasBall()) {
		PPlayer passee = CentralAnalyzingUnit::closestRobot(team.player(2), CentralAnalyzingUnit::TEAM_SAME, false);

		for (unsigned int i = 0; i < Team::SIZE; i++)
			if (team.player(i)->receivingPass())
				passee = team.player(i);
		
		team.player(2)->plan(Plan::passer);
		team.player(2)->otherPlayer(passee);
		passee->receivingPass(true);	
	} else {
		team.player(2)->plan(Plan::move);
		team.player(2)->destination(Vector2(defenseRange, team.player(2)->destination().y));
	}
	
	if (team.player(0)->hasBall()) {
		PPlayer passee = CentralAnalyzingUnit::closestRobot(team.player(0), CentralAnalyzingUnit::TEAM_SAME, true);

		for (unsigned int i = 0; i < Team::SIZE; i++)
			if (team.player(i)->receivingPass())
				passee = team.player(i);

		team.player(0)->plan(Plan::passer);
		team.player(0)->otherPlayer(passee);
		passee->receivingPass(true);
	} else {
		team.player(0)->plan(Plan::goalie);
	}
}

