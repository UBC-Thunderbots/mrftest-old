#include <tr1/memory>
class Strategy;
typedef std::tr1::shared_ptr<Strategy> PStrategy;

#ifndef AI_STRATEGIES_STRATEGY_H
#define AI_STRATEGIES_STRATEGY_H

#include "datapool/Noncopyable.h"

class AITeam;
class Strategy : private virtual Noncopyable {
public:
	Strategy(AITeam &team) : team(team) {}
	virtual ~Strategy() {}
	virtual void update() = 0;

protected:
	AITeam &team;
};

#endif

