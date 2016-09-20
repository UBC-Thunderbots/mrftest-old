#include "ai/flags.h"
#include <stdexcept>

AI::Flags::MoveFlags AI::Flags::calc_flags(AI::Common::PlayType pt) {
	// All robots want to avoid the defence area (except for the goalie).
	AI::Flags::MoveFlags flags = AI::Flags::MoveFlags::AVOID_FRIENDLY_DEFENSE | AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE;
	switch (pt) {
		case AI::Common::PlayType::STOP:
		case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
		case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
		case AI::Common::PlayType::BALL_PLACEMENT_ENEMY:
		case AI::Common::PlayType::BALL_PLACEMENT_FRIENDLY:
			flags |= AI::Flags::MoveFlags::AVOID_BALL_STOP;
			return flags;

		case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
		case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
		case AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY:
			flags |= AI::Flags::MoveFlags::AVOID_BALL_STOP;
			flags |= AI::Flags::MoveFlags::STAY_OWN_HALF;
			return flags;

		case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
		case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
			flags |= AI::Flags::MoveFlags::AVOID_BALL_STOP;
			flags |= AI::Flags::MoveFlags::PENALTY_KICK_ENEMY;
			return flags;

		case AI::Common::PlayType::NONE:
			return AI::Flags::MoveFlags::NONE;

		default:
			return flags;
	}
}
