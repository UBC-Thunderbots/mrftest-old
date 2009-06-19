#include "datapool/Config.h"
#include "datapool/RobotMap.h"
#include "datapool/Team.h"
#include "datapool/World.h"
#include "Log/Log.h"

#include <vector>
#include <string>
#include <sstream>
#include <cassert>
#include <climits>
#include <glibmm.h>

RobotMap::RobotMap() {
	// Only one instance can exist at a time.
	assert(!inst);

	// Initialize the mapping.
	static const std::string teamPrefixes[2] = {"Friendly", "Enemy"};
	log2phys.resize(2 * Team::SIZE, UINT_MAX);
	for (unsigned int t = 0; t < 2; t++)
		for (unsigned int i = 0; i < Team::SIZE; i++) {
			std::ostringstream oss;
			oss << teamPrefixes[t] << i;
			std::string key = oss.str();
			if (Config::instance().hasKey("Mapping", key)) {
				unsigned int phys = Config::instance().getInteger<unsigned int>("Mapping", key, 10);
				if (phys2log.size() <= phys)
					phys2log.resize(phys + 1, UINT_MAX);
				phys2log[phys] = t * Team::SIZE + i;
				log2phys[t * Team::SIZE + i] = phys;
			}
		}

	// Record the global instance.
	inst = this;
}

RobotMap::~RobotMap() {
	// Record destruction.
	inst = 0;
}

RobotMap &RobotMap::instance() {
	return *inst;
}

unsigned int RobotMap::l2p(PPlayer plr) {
	unsigned int id = plr->id();
	assert(id < log2phys.size());
	return log2phys[id];
}

PPlayer RobotMap::p2l(unsigned int pid) {
	if (pid < phys2log.size()) {
		unsigned int lid = phys2log[pid];
		return World::get().team(lid / Team::SIZE).player(lid % Team::SIZE);
	} else {
		return PPlayer();
	}
}

RobotMap *RobotMap::inst;

