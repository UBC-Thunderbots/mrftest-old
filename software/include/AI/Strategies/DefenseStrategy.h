#ifndef DEFENSESTRATEGY_H_
#define DEFENSESTRATEGY_H_

#include "AI/Strategies/Strategy.h"

#include "AI/AITeam.h"
#include "datapool/World.h"
#include "AI/RobotController.h"


class DefenseStrategy : public Strategy {
public:
	DefenseStrategy(AITeam &team);	
	virtual ~DefenseStrategy();
	virtual void update();

private:
	DefenseStrategy(const DefenseStrategy &copyref); // Prohibit copying.
	void init();
	void defense();
	
	PField field;
	PTeam pTeam;
	PTeam pOther;
	PBall pBall;
	bool isUsed[5];
	
	
	
};

#endif /*DEFENSESTRATEGY_H_*/
