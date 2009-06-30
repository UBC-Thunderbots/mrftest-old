#ifndef AI_STRATEGIES_SHOOTINGSTRATEGY_H
#define AI_STRATEGIES_SHOOTINGSTRATEGY_H

#include "AI/Strategies/Strategy.h"
#include "datapool/Noncopyable.h"

class ShootingChallengeStrategy : public Strategy, private virtual Noncopyable {
public:
	ShootingChallengeStrategy(AITeam &team);	
	virtual void update();
};

#endif

