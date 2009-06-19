#ifndef AI_STRATEGIES_OFFENSIVESTRATEGY_H
#define AI_STRATEGIES_OFFENSIVESTRATEGY_H

#include "AI/Strategies/Strategy.h"
#include "datapool/Noncopyable.h"

class OffensiveStrategy : public Strategy, private virtual Noncopyable {
public:
	OffensiveStrategy(AITeam &team);
	virtual void update();
	
private:
	double defenseRange;
};

#endif

