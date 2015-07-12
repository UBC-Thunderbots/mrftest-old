#ifndef AI_HL_STP_PLAYEXECUTOR_H
#define AI_HL_STP_PLAYEXECUTOR_H

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/play/play.h"
#include <string>
#include "ai/hl/stp/gradient_approach/PassInfo.h"

namespace AI {
	namespace HL {
		namespace STP {
			
			/**
			 * These values should purely be used to record the active tactics used. 
			 */
			namespace Active {
				extern Player active_player;
				extern Player last_kicked;
			}
			/**
			 * A play executor.
			 * See STP paper section 5.3.
			 */
			 
			class PlayExecutor : public sigc::trackable {
				public:
					explicit PlayExecutor(World w);

					/**
					 * Runs every time step.
					 */
					void tick();

					/**
					 * Get a text string about the executor.
					 */
					Glib::ustring info() const;

				protected:
					World world;

					/**
					 * If index of i is true, then the robot of pattern index i is enabled. 
					 * If a robot is enabled it is considered by the play executor to be 
					 * within the pool of available players. 
					 */
					std::vector<bool> players_enabled;

					/**
					 * \brief Idle tactics to use when other tactics have not yet been selected.
					 */
					Tactic::Tactic::Ptr idle_tactics[TEAM_MAX_SIZE];

					/**
					 * The play in use currently.
					 */
					Play::Play *curr_play;
					
					/**
					 * indicates which step in the role we are using.
					 */
					unsigned int curr_role_step;

					/**
					 * A role is a sequence of tactics.
					 * The first role is for goalie.
					 */
					std::vector<Tactic::Tactic::Ptr> curr_roles[TEAM_MAX_SIZE];

					/**
					 * The tactic in use
					 */
					Tactic::Tactic *curr_tactic[TEAM_MAX_SIZE];

					/**
					 * Active tactic in use.
					 */
					Tactic::Tactic *curr_active;

					// current player assignment
					AI::HL::W::Player curr_assignment[TEAM_MAX_SIZE];

					// previous player assignment
					AI::HL::W::Player prev_assignment[TEAM_MAX_SIZE];

					/**
					 * List of all the available plays
					 */
					std::vector<std::unique_ptr<Play::Play>> plays;

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

					void clear_assignments();
					
					/**
					 * used to init players_enabled
					 */
					void enable_players();
			};
		};
	}
}

#endif

