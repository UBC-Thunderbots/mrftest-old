#ifndef AI_SIMULATOR_H
#define AI_SIMULATOR_H

#include "datapool/Noncopyable.h"
#include "datapool/Team.h"
#include "datapool/Updateable.h"

class Simulator : private virtual Noncopyable, public virtual Updateable {
public:
	Simulator(Team &friendly, Team &enemy);
	void update();
};

#endif

