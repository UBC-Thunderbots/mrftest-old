#ifndef DATAPOOL_PLAN_H
#define DATAPOOL_PLAN_H

namespace Plan {  
	enum Behavior {
		stop,
      	passer, 	// to player index i
      	shoot,
      	chase,
      	move,		// to position x,y
      	goalie,
      	defender,
      	guard		// player i
   	};
}

#endif

