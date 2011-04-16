#ifndef AI_HL_STP_PLAYEXECUTOR_H
#define AI_HL_STP_PLAYEXECUTOR_H

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/play/play.h"

#include <gtkmm.h>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * A play executor.
			 * See STP paper section 5.3.
			 */
			class PlayExecutor {
				public:
					PlayExecutor(World &w);

					/**
					 * Runs every time step.
					 */
					void tick();

				protected:
					World &world;

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
					 * Calculates a NEW play to be used.
					 */
					virtual void calc_play();

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

					void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx);
			};
		};
	}
}

#endif

