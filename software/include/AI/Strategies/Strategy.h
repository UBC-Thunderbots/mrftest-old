#include <tr1/memory>
class Strategy;
typedef std::tr1::shared_ptr<Strategy> PStrategy;

#ifndef TB_STRATEGY_H
#define TB_STRATEGY_H

class AITeam;
class Strategy {
public:
	Strategy(AITeam &team) : team(team) {}
	virtual ~Strategy() {}
	virtual void update() = 0;

protected:
	AITeam &team;

private:
	Strategy(const Strategy &copyref); // Prohibit copying.
};

#endif

