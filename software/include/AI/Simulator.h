#ifndef AI_SIMULATOR_H
#define AI_SIMULATOR_H

#include "AI/AITeam.h"
#include "datapool/Noncopyable.h"

class Simulator : private virtual Noncopyable {
public:
	Simulator();
	void update();

private:
	AITeam friendly, enemy;
};

#endif

