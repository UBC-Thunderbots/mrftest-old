#ifndef AI_STRATEGIES_CHASESTRATEGY_H
#define AI_STRATEGIES_CHASESTRATEGY_H

#include "AI/Strategies/Strategy.h"
#include "datapool/Noncopyable.h"

class ChaseStrategy : public Strategy, private virtual Noncopyable {
public:
	ChaseStrategy(AITeam &team);	
	virtual void update();
};

#endif

