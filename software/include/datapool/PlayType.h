#ifndef DATAPOOL_PLAYTYPE_H
#define DATAPOOL_PLAYTYPE_H

namespace PlayType {
	enum Type {
		doNothing,			// do nothing
		start,				// get in starting formation
		stop,
		play,				// play is in progress
		directFreeKick,
		indirectFreeKick,
		kickoff,
		preparePenaltyKick,
		penaltyKick,
		penaltyKickoff,
		victoryDance,
		prepareKickoff
	};
}

#endif

