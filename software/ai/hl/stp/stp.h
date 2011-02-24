#ifndef AI_HL_STP_STP_H
#define AI_HL_STP_STP_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/tactic.h"

#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			using namespace AI::HL::W;

			/**
			 * The state of the current STP
			 * TODO: this is just a rough idea of what it will look like.
			 */
			class STP {
				public:
					/**
					 * The current play selected.
					 */
					virtual const Play::CPtr play() const = 0;

					/**
					 * Let i be the value returned.
					 * Then the i-th tactic is run in every role.
					 */
					virtual unsigned tactic_step() const = 0;

					/**
					 * The i-th list of tactics.
					 */
					virtual const std::vector<Tactic::CPtr>& tactics(unsigned i) const = 0;

					/**
					 * The i-th index is the active tactic.
					 */
					virtual unsigned active_index() const = 0;

					/**
					 * The current tactic in use.
					 */
					virtual Tactic::CPtr curr_tactic(unsigned i) const = 0;

					/**
					 * The current player in the i-th tactic.
					 */
					virtual Player::CPtr player(unsigned i) const = 0;
			};

			/**
			 * Provides information about the running STP, as well as HL world.
			 */
			class STPWorld {
				public:
					/**
					 * Returns the field.
					 *
					 * \return the field.
					 */
					virtual const Field &field() const = 0;

					/**
					 * Returns the ball.
					 *
					 * \return the ball.
					 */
					virtual const Ball &ball() const = 0;

					/**
					 * Returns the friendly team.
					 *
					 * \return the friendly team.
					 */
					virtual const FriendlyTeam &friendly_team() const = 0;

					/**
					 * Returns the enemy team.
					 *
					 * \return the enemy team.
					 */
					virtual const EnemyTeam &enemy_team() const = 0;

					/**
					 * Returns the current play type.
					 *
					 * \return the current play type.
					 */
					virtual const Property<PlayType::PlayType> &playtype() const = 0;

					/**
					 * Returns the current STP state.
					 *
					 * \return the current STP state.
					 */
					virtual const STP& stp() const = 0;
			};
		}
	}
}

#endif

