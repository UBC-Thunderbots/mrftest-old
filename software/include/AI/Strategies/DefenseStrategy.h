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

	void init();
	void defense();
	void nearDefense();
	void farDefense();

	// assign n defenders to n positions
	// n! runtime
	void assignDefenders(const Vector2* blockPosition, int n);
};

#endif

