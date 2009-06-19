#ifndef AI_STRATEGIES_LAZYSTRATEGY_H
#define AI_STRATEGIES_LAZYSTRATEGY_H

#include "AI/Strategies/Strategy.h"
#include "datapool/Noncopyable.h"

class LazyStrategy : public Strategy, private virtual Noncopyable {
public:
	LazyStrategy(AITeam &team);	
	virtual void update();
};

#endif

