#ifndef TB_TESTSTRATEGY_H
#define TB_TESTSTRATEGY_H

#include "AI/Strategies/Strategy.h"

class TestStrategy : public Strategy {
public:
	TestStrategy(AITeam &team);	
	virtual ~TestStrategy();
	virtual void update();
};

#endif

