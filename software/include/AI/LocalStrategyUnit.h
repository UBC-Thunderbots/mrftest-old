//LocalStrategyUnit: Bottom layer of strategy creation.
//Used for path finding, obstacle avoidance, and carrying out robot commands
//Communicates robot actions to the controllers.

#ifndef TB_LOCALSTRATEGYUNIT_H
#define TB_LOCALSTRATEGYUNIT_H

#include "datapool/Player.h"

class AITeam;
class LocalStrategyUnit {
public:
	LocalStrategyUnit(AITeam &team);
	void update();

private:
	LocalStrategyUnit(const LocalStrategyUnit &copyref); // Prohibit copying.
	AITeam &team;
	void stop(PPlayer robot);
	void shoot(PPlayer robot);
	void pass(PPlayer robot);
	void passee(PPlayer robot);
	void chaseBall(PPlayer robot);
	void dribble(PPlayer robot);
	void move(PPlayer robot, Vector2 pos);
	void goalie(PPlayer robot);
};

#endif

