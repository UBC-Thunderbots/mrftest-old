class CentralStrategyUnit;

#ifndef TB_CENTRALSTRATEGYUNIT_H
#define TB_CENTRALSTRATEGYUNIT_H

#include "Strategies/Strategy.h"

class AITeam;
class CentralStrategyUnit {
public:
	CentralStrategyUnit(AITeam &team);
	void update();
	PStrategy strategy() const;
	void strategy(const PStrategy &s);

private:
	CentralStrategyUnit(const CentralStrategyUnit &copyref); // Prohibit copying.
	AITeam &team;
	PStrategy strat;

	void startingPositions(); // Move all the players to their starting positions.
	void stop();
	void directFreeKick();
	void indirectFreeKick();
	void preparePenaltyKick();
	void penaltyKick();
	void kickoff();
	void penaltyKickoff();
	void victoryDance();
	void doNothing();
	void prepareKickoff();
};

#endif

