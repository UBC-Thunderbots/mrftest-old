#ifndef DEFENSESTRATEGY_H_
#define DEFENSESTRATEGY_H_

#include "AI/Strategies/Strategy.h"

#include "AI/AITeam.h"
#include "datapool/World.h"
#include "AI/RobotController.h"


class DefenseStrategy : public Strategy {
public:
	DefenseStrategy(AITeam &team);	
	virtual void update();

private:
	bool isUsed[5];

	DefenseStrategy(const DefenseStrategy &copyref); // Prohibit copying.
	void init();
	void defense();
};

#endif /*DEFENSESTRATEGY_H_*/
