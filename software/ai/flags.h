#ifndef AI_FLAGS_H
#define AI_FLAGS_H

#include "ai/world/playtype.h"

namespace AIFlags {
	/**
	 * Navigator flags for special rules.
	 * Roles set flags in tactics, and tactics pass the flags to navigators.
	 */
	enum {
		CLIP_PLAY_AREA         = 0x0001, /**< enum Force robot to stay in play area. */
		AVOID_BALL_NEAR        = 0x0002, /**< enum Avoid ball for pivoting (probably used for free kick). */
		AVOID_BALL_STOP        = 0x0004, /**< enum Avoid ball when stop is in play. */
		AVOID_FRIENDLY_DEFENSE = 0x0008, /**< enum Avoid friendly defence area. */
		AVOID_ENEMY_DEFENSE    = 0x0010, /**< enum Avoid enemy defence area. */
		STAY_OWN_HALF          = 0x0020, /**< enum Stay in your own half. */
		PENALTY_KICK_FRIENDLY  = 0x0040, /**< enum Neither goalie nor kicker. Stay away at certain boundary. */
		PENALTY_KICK_ENEMY     = 0x0080, /**< enum Neither goalie nor kicker. Stay away at certain boundary. */
	};
	
	namespace {
		/**
		* Returns the correct flags for a common Player,
		* i.e. a Player that is NOT a goalie or the free kicker.
		* Assume our team does not have the ball, unless the play type indicates otherwise.
		* The flags for those should be modified in the corresponding roles.
		*/
		unsigned int calc_flags(PlayType::PlayType pt){
			// All robots want to avoid the defence area (except for the goalie)
			unsigned int flags = AVOID_FRIENDLY_DEFENSE;
			switch(pt){
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
			    
			  default: break;
			}
			return flags;
		}
	}
}

#endif
