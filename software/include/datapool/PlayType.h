#ifndef DATAPOOL_PLAYTYPE_H
#define DATAPOOL_PLAYTYPE_H

namespace PlayType {
	// =======
	// WARNING
	// =======
	//
	// Do NOT, under ANY CIRCUMSTANCES, reorder, delete, or insert new play types.
	// New play types can only be added at the _end_ of the list.
	// Yes, this means after the "noType" value.
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
		prepareKickoff,
		noType,
	};
}

#endif

