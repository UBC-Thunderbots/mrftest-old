#ifndef AI_STRATEGIES_DEFENSESTRATEGY_H
#define AI_STRATEGIES_DEFENSESTRATEGY_H

#include "AI/AITeam.h"
#include "AI/RobotController.h"
#include "AI/Strategies/Strategy.h"
#include "datapool/Noncopyable.h"
#include "datapool/World.h"


class DefenseStrategy : public Strategy, private virtual Noncopyable {
public:
	DefenseStrategy(AITeam &team);	
	virtual void update();

private:
	bool isUsed[5];
	bool attackerWaiting;
	unsigned int oldAttacker;

	void init();
	void defense();
	void nearDefense();
	void farDefense();

	// assign n defenders to n positions
	// n! runtime
	// will modify blockPosition if the distance is too close to goal radius
	void assignDefenders(Vector2* blockPosition, int n);
	
	// copied from Dibbs strategy
	void goalie(PPlayer robot);
	void attacker(PPlayer attacker, PPlayer supporter);
	void supporter(PPlayer attacker, PPlayer supporter);
};

#endif

