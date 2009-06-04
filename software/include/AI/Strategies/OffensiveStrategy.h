#ifndef TB_OFFENSIVESTRATEGY_H
#define TB_OFFENSIVESTRATEGY_H

#include "AI/Strategies/Strategy.h"

class OffensiveStrategy : public Strategy {
public:
	OffensiveStrategy(AITeam &team);
	virtual ~OffensiveStrategy();
	virtual void update();
	
private:
	double defenseRange;
};

#endif

