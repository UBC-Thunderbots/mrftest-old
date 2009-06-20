#ifndef AI_SIMULATOR_H
#define AI_SIMULATOR_H

#include "AI/AITeam.h"
#include "datapool/Noncopyable.h"
#include "datapool/Updateable.h"

class Simulator : private virtual Noncopyable, public virtual Updateable {
public:
	Simulator();
	void update();

private:
	AITeam friendly, enemy;
};

#endif

