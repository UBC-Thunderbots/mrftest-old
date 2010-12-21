#ifndef AI_HL_STP_PLAY_PLAY_H
#define AI_HL_STP_PLAY_PLAY_H

#include "ai/hl/world.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <ctime>
#include <sigc++/sigc++.h>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Play {
				/**
				 * A play is a level in the STP paradigm.
				 */
				class Play : public ByRef, public sigc::trackable {
					public:
						typedef RefPtr<Play> Ptr;

						/**
						 * assign the roles.
						 */
						virtual void assign(std::vector<AI::HL::STP::Tactic::Tactic::Ptr> &goalie_role, std::vector<AI::HL::STP::Tactic::Tactic::Ptr> &role1, std::vector<AI::HL::STP::Tactic::Tactic::Ptr> &role2, std::vector<AI::HL::STP::Tactic::Tactic::Ptr> &role3, std::vector<AI::HL::STP::Tactic::Tactic::Ptr> &role4) = 0;

						/**
						 * Checks if the condition for the play is no longer valid.
						 */
						virtual bool done() = 0;

						/**
						 * Indicates how likely tactics assignments are changed.
						 *
						 * \return the probability that role change is allowed.
						 */
						double change_probability() const;

						/**
						 * Time that this play is allowed to run.
						 */
						double timeout() const;

						/**
						 * Check if the play has aborted.
						 */
						bool aborted() const;

						/**
						 * Calculates how much time has elapsed.
						 */
						double elapsed_time() const;

					protected:
						/**
						 * The World in which the Play lives.
						 */
						AI::HL::W::World &world;

						Play(AI::HL::W::World &world);

						/**
						 * Destructor
						 */
						virtual ~Play();

						/**
						 * A subclass can invoke this function to abort.
						 */
						void abort();

						/**
						 * A subclass can call this to change the timeout.
						 */
						void set_timeout(const double t);

						/**
						 * A subclass can call this to change the frequency of player assignments.
						 */
						void set_change_probability(const double p);

					private:
						bool aborted_;
						double timeout_;
						double change_probability_;
						std::time_t start_time;
				};

				/**
				 * A PlayManager is used to manage a particular play.
				 * It has two objectives:
				 * - check the initial condition
				 * - factory to create an instance of play
				 */
				class PlayManager : public ByRef, public Registerable<PlayManager> {
					public:
						typedef RefPtr<PlayManager> Ptr;

						/**
						 * Check if this play should be selected.
						 */
						virtual bool applicable(AI::HL::W::World &world) const = 0;

						/**
						 * Constructs a new instance of the Play corresponding to this PlayManager.
						 *
						 * \param[in] world the World in which the new Play should live.
						 *
						 * \return the new Play.
						 */
						virtual Play::Ptr create_play(AI::HL::W::World &world) const = 0;

					protected:
						/**
						 * Constructs a new PlayManager.
						 * Subclasses should call this constructor from their own constructors.
						 *
						 * \param[in] name a human-readable name for this Strategy.
						 */
						PlayManager(const char *name);

						~PlayManager();
				};
			}
		}
	}
}

#endif

