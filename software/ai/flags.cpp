#include "ai/flags.h"
#include <stdexcept>

const unsigned int AI::Flags::FLAGS_VALID =
    FLAG_CLIP_PLAY_AREA |
    FLAG_AVOID_BALL_STOP |
    FLAG_AVOID_BALL_TINY |
    FLAG_AVOID_FRIENDLY_DEFENSE |
    FLAG_AVOID_ENEMY_DEFENSE |
    FLAG_STAY_OWN_HALF |
    FLAG_PENALTY_KICK_FRIENDLY |
    FLAG_PENALTY_KICK_ENEMY |
    FLAG_FRIENDLY_KICK;

unsigned int AI::Flags::calc_flags(AI::Common::PlayType pt) {
	// All robots want to avoid the defence area (except for the goalie).
	unsigned int flags = FLAG_AVOID_FRIENDLY_DEFENSE;
	switch (pt) {
		case AI::Common::PlayType::HALT:
		case AI::Common::PlayType::PLAY:
			return flags;

		case AI::Common::PlayType::STOP:
		case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
		case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_AVOID_ENEMY_DEFENSE;
			return flags;

		case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
		case AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY:
		case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
		case AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_STAY_OWN_HALF;
			return flags;

		case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
		case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_AVOID_ENEMY_DEFENSE;
			flags |= FLAG_FRIENDLY_KICK;
			return flags;

		case AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY:
		case AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_AVOID_ENEMY_DEFENSE;
			flags |= FLAG_PENALTY_KICK_FRIENDLY;
			return flags;

		case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
		case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
			flags |= FLAG_AVOID_BALL_STOP;
			flags |= FLAG_AVOID_ENEMY_DEFENSE;
			flags |= FLAG_PENALTY_KICK_ENEMY;
			return flags;

		case AI::Common::PlayType::NONE:
			break;
	}
	throw std::out_of_range("Play type number is out of range!");
}

