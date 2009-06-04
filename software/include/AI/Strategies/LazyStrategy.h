#ifndef TB_LAZYSTRATEGY_H
#define TB_LAZYSTRATEGY_H

#include "AI/Strategies/Strategy.h"

class LazyStrategy : public Strategy {
public:
	LazyStrategy(AITeam &team);	
	virtual ~LazyStrategy();
	virtual void update();

private:
	LazyStrategy(const LazyStrategy &copyref); // Prohibit copying.
};

#endif
