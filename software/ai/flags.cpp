#include "ai/flags.h"
#include <stdexcept>

namespace PlayType = AI::Common::PlayType;

unsigned int AI::Flags::calc_flags(PlayType::PlayType pt) {
	// All robots want to avoid the defence area (except for the goalie).
	unsigned int flags = FLAG_AVOID_FRIENDLY_DEFENSE;
	switch (pt) {
		case PlayType::HALT:
		case PlayType::PLAY:
			return flags;

		case PlayType::STOP:
		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_AVOID_ENEMY_DEFENSE;
			return flags;

		case PlayType::PREPARE_KICKOFF_FRIENDLY:
		case PlayType::EXECUTE_KICKOFF_FRIENDLY:
		case PlayType::PREPARE_KICKOFF_ENEMY:
		case PlayType::EXECUTE_KICKOFF_ENEMY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_STAY_OWN_HALF;
			return flags;

		case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
		case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_AVOID_ENEMY_DEFENSE;
			return flags;

		case PlayType::PREPARE_PENALTY_FRIENDLY:
		case PlayType::EXECUTE_PENALTY_FRIENDLY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_AVOID_ENEMY_DEFENSE;
			flags |= FLAG_PENALTY_KICK_FRIENDLY;
			return flags;

		case PlayType::PREPARE_PENALTY_ENEMY:
		case PlayType::EXECUTE_PENALTY_ENEMY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_AVOID_ENEMY_DEFENSE;
			flags |= FLAG_PENALTY_KICK_ENEMY;
			return flags;

		case PlayType::COUNT:
			break;
	}
	throw std::out_of_range("Play type number is out of range!");
}

