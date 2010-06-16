#ifndef AI_FLAGS_H
#define AI_FLAGS_H

#include "ai/world/playtype.h"

namespace ai_flags {
	/**
	 * Navigator flags for special rules.
	 * Roles set flags in tactics, and tactics pass the flags to navigators.
	 */
	enum {
		clip_play_area         = 0x0001, /**< enum Force robot to stay in play area. */
		avoid_ball_near        = 0x0002, /**< enum Avoid ball for pivoting (probably used for free kick). */
		avoid_ball_stop        = 0x0004, /**< enum Avoid ball when stop is in play. */
		avoid_friendly_defence = 0x0008, /**< enum Avoid friendly defence area. */
		avoid_enemy_defence    = 0x0010, /**< enum Avoid enemy defence area. */
		stay_own_half          = 0x0020, /**< enum Stay in your own half. */
		penalty_kick_friendly  = 0x0040, /**< enum Neither goalie nor kicker. Stay away at certain boundary. */
		penalty_kick_enemy     = 0x0080, /**< enum Neither goalie nor kicker. Stay away at certain boundary. */
	};
	
	namespace {
		/**
		* Returns the correct flags for a common player,
		* i.e. a player that is NOT a goalie or the free kicker.
		* Assume our team does not have the ball, unless the play type indicates otherwise.
		* The flags for those should be modified in the corresponding roles.
		*/
		unsigned int calc_flags(playtype::playtype pt){
			// All robots want to avoid the defence area (except for the goalie)
			unsigned int flags = avoid_friendly_defence;
			flags |= avoid_enemy_defence;
			switch(pt){
			  case playtype::stop:
			  case playtype::execute_direct_free_kick_enemy:
			  case playtype::execute_indirect_free_kick_enemy:
			    flags |= avoid_ball_stop;
			    break;
			    
			  case playtype::prepare_kickoff_enemy:
			  case playtype::execute_kickoff_enemy:
			    flags |= avoid_ball_stop;
			    flags |= stay_own_half;
			    break;
			    
			  case playtype::prepare_kickoff_friendly:
			  case playtype::execute_kickoff_friendly:
			    flags |= stay_own_half;
			    flags |= clip_play_area;
			    flags |= avoid_ball_stop;
				flags &= ~avoid_enemy_defence;
			    break;
			  
			  case playtype::execute_direct_free_kick_friendly:
			  case playtype::execute_indirect_free_kick_friendly:
			    flags |= avoid_enemy_defence;
			    flags |= clip_play_area;
			    break;
			  
			  case playtype::prepare_penalty_friendly:
			  case playtype::execute_penalty_friendly:
			    flags |= penalty_kick_friendly;
				flags &= ~avoid_enemy_defence;
			    break;
			    
			  case playtype::prepare_penalty_enemy:
			  case playtype::execute_penalty_enemy:
			    flags |= penalty_kick_enemy;
			    break;
			    
			  default: break;
			}
			return flags;
		}
	}
}

#endif
