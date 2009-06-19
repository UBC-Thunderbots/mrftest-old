#ifndef DATAPOOL_ROBOTMAP_H
#define DATAPOOL_ROBOTMAP_H

#include "datapool/Player.h"

#include <vector>

//
// Maintains the mapping between physical (vision and XBee) and logical (AI) robot ID numbers.
//
class RobotMap {
public:
	//
	// Creates a new RobotMap. Only one instance may exist at a time.
	//
	RobotMap();

	//
	// Destroys the RobotMap.
	//
	~RobotMap();

	//
	// Gets the currently-existing instance.
	//
	static RobotMap &instance();

	//
	// Maps a player to a physical ID.
	//
	unsigned int l2p(PPlayer plr);

	//
	// Maps a physical ID to a player.
	//
	PPlayer p2l(unsigned int id);

private:
	std::vector<unsigned int> phys2log;
	std::vector<unsigned int> log2phys;
	static RobotMap *inst;
};

#endif

