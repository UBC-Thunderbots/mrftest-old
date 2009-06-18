#ifndef TB_CHEATERSTRATEGY_H
#define TB_CHEATERSTRATEGY_H

#include "AI/Strategies/Strategy.h"
#include "datapool/Player.h"

class CheaterStrategy : public Strategy {
public:
	CheaterStrategy(AITeam &team);	
	virtual ~CheaterStrategy();
	virtual void update();

private:
	CheaterStrategy(const CheaterStrategy &copyref); // Prohibit copying.
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

