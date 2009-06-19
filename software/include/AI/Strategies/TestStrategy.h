#ifndef AI_STRATEGIES_TESTSTRATEGY_H
#define AI_STRATEGIES_TESTSTRATEGY_H

#include "AI/Strategies/Strategy.h"
#include "datapool/Noncopyable.h"

class TestStrategy : public Strategy, private virtual Noncopyable {
public:
	TestStrategy(AITeam &team);	
	virtual void update();
};

#endif

