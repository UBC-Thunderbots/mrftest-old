#ifndef AI_HL_STP_PLAYEXECUTOR_H
#define AI_HL_STP_PLAYEXECUTOR_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * A play executor.
			 * See STP paper section 5.3.
			 */
			class PlayExecutor {
				public:
					PlayExecutor(AI::HL::W::World& w);
	
					/**
					 * Runs every time step.
					 */
					void tick();

				protected:
					AI::HL::W::World& world;

					/**
					 * The play in use currently.
					 */
					Play::Play::Ptr curr_play;

					/**
					 * indicates which step in the role we are using.
					 */
					unsigned int curr_role_step;

					/**
					 * A role is a sequence of tactics.
					 * The first role is for goalie.
					 */
					std::vector<Tactic::Tactic::Ptr> curr_roles[5];

					/**
					 * The tactic in use
					 */
					Tactic::Tactic::Ptr curr_tactic[5];

					/**
					 * Active tactic in use.
					 */
					Tactic::Tactic::Ptr curr_active;

					// current player assignment
					AI::HL::W::Player::Ptr curr_assignment[5];

					/**
					 * List of all the available plays
					 */
					std::vector<Play::Play::Ptr> plays;

					/**
					 * When something bad happens,
					 * resets the current play.
					 */
					void reset();

					/**
					 * Calculates a NEW play to be used.
					 */
					void calc_play();

					/**
					 * Condition: a play is in use.
					 * Calculates and executes tactics.
					 */
					void execute_tactics();

					/**
					 * Condition: a valid list of tactics.
					 * Dynamically run tactic to play assignment.
					 */
					void role_assignment();

				private:
					void on_player_added(std::size_t);
					void on_player_removed();
			};
		};
	}
}

#endif

