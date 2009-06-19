#ifndef AI_STRATEGIES_DIBSSTRATEGY_H
#define AI_STRATEGIES_DIBSSTRATEGY_H

#include "AI/Strategies/Strategy.h"
#include "datapool/Noncopyable.h"
#include "datapool/Player.h"

class DibsStrategy : public Strategy, private virtual Noncopyable {
public:
	DibsStrategy(AITeam &team);	
	virtual void update();

private:
	bool attackerWaiting;
	unsigned int oldAttacker;
	int waitingCounter;
	void goalie(PPlayer robot);
	void attacker(PPlayer attacker, PPlayer supporter);
	void supporter(PPlayer defender, PPlayer attacker);
	void defenderTop(PPlayer robot);
	void defenderBottom(PPlayer robot);
};

#endif

