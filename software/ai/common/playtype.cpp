#include "ai/common/playtype.h"
#include <cstddef>
#include <cstdlib>
#include <stdexcept>

AI::Common::PlayType AI::Common::PlayTypeInfo::of_int(unsigned int pt) {
	if (pt <= static_cast<unsigned int>(AI::Common::PlayType::NONE)) {
		return static_cast<AI::Common::PlayType>(pt);
	} else {
		throw std::invalid_argument("Invalid play type index");
	}
}

Glib::ustring AI::Common::PlayTypeInfo::to_string(PlayType pt) {
	switch (pt) {
		case PlayType::HALT:
			return "Halt";

		case PlayType::STOP:
			return "Stop";

		case PlayType::PLAY:
			return "Play";

		case PlayType::PREPARE_KICKOFF_FRIENDLY:
			return "Prep Kickoff Friendly";

		case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			return "Kickoff Friendly";

		case PlayType::PREPARE_KICKOFF_ENEMY:
			return "Prep Kickoff Enemy";

		case PlayType::EXECUTE_KICKOFF_ENEMY:
			return "Kickoff Enemy";

		case PlayType::PREPARE_PENALTY_FRIENDLY:
			return "Prep Penalty Friendly";

		case PlayType::EXECUTE_PENALTY_FRIENDLY:
			return "Penalty Friendly";

		case PlayType::PREPARE_PENALTY_ENEMY:
			return "Prep Penalty Enemy";

		case PlayType::EXECUTE_PENALTY_ENEMY:
			return "Penalty Enemy";

		case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			return "Direct Free Friendly";

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			return "Indirect Free Friendly";

		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			return "Direct Free Enemy";

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			return "Indirect Free Enemy";

		case PlayType::NONE:
			return "None";
	}

	std::abort();
}

AI::Common::PlayType AI::Common::PlayTypeInfo::invert(PlayType pt) {
	switch (pt) {
		case PlayType::HALT:
		case PlayType::STOP:
		case PlayType::PLAY:
		case PlayType::NONE:
			return pt;

		case PlayType::PREPARE_KICKOFF_FRIENDLY:
			return PlayType::PREPARE_KICKOFF_ENEMY;

		case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			return PlayType::EXECUTE_KICKOFF_ENEMY;

		case PlayType::PREPARE_KICKOFF_ENEMY:
			return PlayType::PREPARE_KICKOFF_FRIENDLY;

		case PlayType::EXECUTE_KICKOFF_ENEMY:
			return PlayType::EXECUTE_KICKOFF_FRIENDLY;

		case PlayType::PREPARE_PENALTY_FRIENDLY:
			return PlayType::PREPARE_PENALTY_ENEMY;

		case PlayType::EXECUTE_PENALTY_FRIENDLY:
			return PlayType::EXECUTE_PENALTY_ENEMY;

		case PlayType::PREPARE_PENALTY_ENEMY:
			return PlayType::PREPARE_PENALTY_FRIENDLY;

		case PlayType::EXECUTE_PENALTY_ENEMY:
			return PlayType::EXECUTE_PENALTY_FRIENDLY;

		case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			return PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			return PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;

		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			return PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			return PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
	}

	std::abort();
}


