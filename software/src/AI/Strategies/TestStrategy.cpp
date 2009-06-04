#include "AI/Strategies/TestStrategy.h"
#include "AI/CentralAnalyzingUnit.h"
#include "AI/AITeam.h"
#include "datapool/World.h"

TestStrategy::TestStrategy(AITeam &team) : Strategy(team) {
	team.player(0)->plan(Plan::goalie);
	team.player(0)->destination(Vector2(550,230));
	team.player(1)->plan(Plan::shoot);
	team.player(1)->destination(Vector2(550,230));
	team.player(2)->plan(Plan::chase);
	team.player(2)->destination(Vector2(550,230));
	team.player(3)->plan(Plan::move);
	team.player(3)->destination(Vector2(550,230));
	team.player(4)->plan(Plan::move);
	team.player(4)->destination(Vector2(550,230));
}

TestStrategy::~TestStrategy() {
}

void TestStrategy::update() {
	team.player(4)->plan(Plan::move);
	if (team.player(4)->destination().y != 230)
		team.player(4)->destination(Vector2(550, 230));
	team.player(3)->plan(Plan::move);
	if (team.player(3)->destination().y != 230)
		team.player(3)->destination(Vector2(550, 230));
	team.player(2)->plan(Plan::chase);
	if (team.player(2)->destination().y != 230)
		team.player(2)->destination(Vector2(550, 230));
	team.player(1)->plan(Plan::shoot);
	team.player(0)->plan(Plan::goalie);

	Vector2 pos = team.player(4)->position();
	if ((pos.x - 550 < 3 && pos.x - 550 > -3) && (pos.y - 230 < 3 && pos.y - 230 > -3))
		team.player(4)->destination(Vector2(100, 230));
	pos = team.player(3)->position();
	if ((pos.x - 550 < 3 && pos.x - 550 > -3) && (pos.y - 230 < 3 && pos.y - 230 > -3))
		team.player(3)->destination(Vector2(100, 230));
	pos = team.player(2)->position();
	if ((pos.x-550 < 3 && pos.x - 550 > -3) && (pos.y - 230 < 3 && pos.y - 230 > -3))
		team.player(2)->destination(Vector2(100, 230));

	pos = team.player(4)->position();
	if ((pos.x - 100 < 3 && pos.x - 100 > -3) && (pos.y - 230 < 3 && pos.y - 230 > -3))
		team.player(4)->destination(Vector2(550, 230));
	pos = team.player(3)->position();
	if ((pos.x - 100 < 3 && pos.x - 100 > -3) && (pos.y - 230 < 3 && pos.y - 230 > -3))
		team.player(3)->destination(Vector2(550, 230));
	pos = team.player(2)->position();
	if ((pos.x - 100 < 3 && pos.x - 100 > -3) && (pos.y - 230 < 3 && pos.y - 230 > -3))
		team.player(2)->destination(Vector2(550, 230));

	if (team.player(2)->hasBall())
		team.player(2)->plan(Plan::move);
	else
		team.player(2)->plan(Plan::chase);

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

