#ifndef CHASESTRATEGY_H_
#define CHASESTRATEGY_H_

#include "AI/Strategies/Strategy.h"

class ChaseStrategy : public Strategy {
public:
	ChaseStrategy(AITeam &team);	
	virtual ~ChaseStrategy();
	virtual void update();

private:
	ChaseStrategy(const ChaseStrategy &copyref); // Prohibit copying.
};

#endif /*CHASESTRATEGY_H_*/
