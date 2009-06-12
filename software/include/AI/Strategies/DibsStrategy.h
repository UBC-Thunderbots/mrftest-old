#ifndef TB_DIBSSTRATEGY_H
#define TB_DIBSSTRATEGY_H

#include "AI/Strategies/Strategy.h"
#include "datapool/Player.h"

class DibsStrategy : public Strategy {
public:
	DibsStrategy(AITeam &team);	
	virtual ~DibsStrategy();
	virtual void update();

private:
	DibsStrategy(const DibsStrategy &copyref); // Prohibit copying.
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

