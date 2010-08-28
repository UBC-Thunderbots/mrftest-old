#include "ai/flags.h"

unsigned int AIFlags::calc_flags(PlayType::PlayType pt) {
	// All robots want to avoid the defence area (except for the goalie)
	unsigned int flags = AVOID_FRIENDLY_DEFENSE;
	switch(pt) {
		case PlayType::STOP:
		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			flags |= AVOID_BALL_STOP;
			flags |= AVOID_ENEMY_DEFENSE;
			break;

		case PlayType::PREPARE_KICKOFF_ENEMY:
		case PlayType::EXECUTE_KICKOFF_ENEMY:
			flags |= AVOID_BALL_STOP;
			flags |= STAY_OWN_HALF;
			break;

		case PlayType::PREPARE_KICKOFF_FRIENDLY:
		case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			flags |= STAY_OWN_HALF;
			flags |= CLIP_PLAY_AREA;
			flags |= AVOID_BALL_STOP;
			break;

		case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
		case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			flags |= AVOID_ENEMY_DEFENSE;
			flags |= CLIP_PLAY_AREA;
			flags |= AVOID_BALL_STOP;
			break;

		case PlayType::PREPARE_PENALTY_FRIENDLY:
		case PlayType::EXECUTE_PENALTY_FRIENDLY:
			flags |= PENALTY_KICK_FRIENDLY;
			flags |= AVOID_BALL_STOP;
			break;

		case PlayType::PREPARE_PENALTY_ENEMY:
		case PlayType::EXECUTE_PENALTY_ENEMY:
			flags |= PENALTY_KICK_ENEMY;
			flags |= AVOID_BALL_STOP;
			break;

		default:
			break;
	}
	return flags;
}

