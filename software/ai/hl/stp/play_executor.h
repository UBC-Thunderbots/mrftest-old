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
			 * A play executor.
			 * See STP paper section 5.3.
			 */
			 
			class PlayExecutor : public sigc::trackable {
				public:
					explicit PlayExecutor(World w);
					~PlayExecutor();

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
					 * The play in use currently.
					 */
					std::unique_ptr<Play::Play> curr_play;

					/**
					 * List of all the available plays
					 */
					std::vector<Play::PlayFactory*> plays;

					/**
					 * Calculates a NEW play to be used.
					 */
					virtual void calc_play();

					void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx);

					void clear_assignments();

					void enable_players();
			};
		};
	}
}

#endif

