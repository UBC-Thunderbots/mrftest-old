//LocalStrategyUnit: Bottom layer of strategy creation.
//Used for path finding, obstacle avoidance, and carrying out robot commands
//Communicates robot actions to the controllers.

#ifndef AI_LOCALSTRATEGYUNIT_H
#define AI_LOCALSTRATEGYUNIT_H

#include "datapool/Noncopyable.h"
#include "datapool/Player.h"

class AITeam;
class LocalStrategyUnit : private virtual Noncopyable {
public:
	LocalStrategyUnit(AITeam &team);
	void update();

private:
	AITeam &team;
	void stop(PPlayer robot);
	void shoot(PPlayer robot);
	void pass(PPlayer robot);
	void passee(PPlayer robot);
	void chaseBall(PPlayer robot);
	void dribble(PPlayer robot);
	void move(PPlayer robot, Vector2 pos, double speed = 0); // speed is between 0 and 1 (0 = slow, 1 = fast)
	void goalie(PPlayer robot);
	bool pivot(PPlayer robot, double angle, Vector2 center); // added by Cedric
};

#endif

