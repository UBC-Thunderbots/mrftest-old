#ifndef AI_HL_STP_PLAY_PLAY_H
#define AI_HL_STP_PLAY_PLAY_H

#include "ai/hl/world.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <sigc++/sigc++.h>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				class Module;
			}

			namespace Play {
				class PlayFactory;

				/**
				 * A play is a level in the STP paradigm.
				 * The purpose of a play is to produce roles.
				 * A role is a sequence of tactic.
				 *
				 * Note that there is only one instance per class.
				 * Plays are reused, destroyed only when the AI ends.
				 * Doing so will simplify coding.
				 *
				 * Please see the instructions in the assign() function.
				 */
				class Play : public ByRef, public sigc::trackable {
					public:
						typedef RefPtr<Play> Ptr;

						/**
						 * Checks if this play is applicable.
						 * A subclass must implement this function.
						 * Be reminded that you have to check the playtype.
						 */
						virtual bool applicable() const = 0;

						/**
						 * Checks if this play has succeeded.
						 * A subclass must implement this function.
						 */
						virtual bool done() const = 0;

						/**
						 * Checks if this play has failed.
						 * A subclass must implement this function.
						 */
						virtual bool fail() const = 0;

						/**
						 * Provide lists of tactics.
						 * Called when this play is initially activated.
						 *
						 * A subclass must implement this function.
						 *
						 * IMPORTANT Conditions
						 * - The roles need not be the same length;
						 *   if a role is shorter than the rest,
						 *   the last tactic is repeated.
						 * - There must be exactly ONE active tactic at any given time.
						 * - An active tactic cannot be repeated.
						 *
						 * \param [in] goalie_role role for the goalie
						 *
						 * \param [in] roles an array of roles in order of priority,
						 * the first entry is the most important etc.
						 */
						virtual void assign(std::vector<AI::HL::STP::Tactic::Tactic::Ptr> &goalie_role, std::vector<AI::HL::STP::Tactic::Tactic::Ptr>(&roles)[4]) = 0;

						/**
						 * A reference to this play's factory.
						 */
						virtual const PlayFactory &factory() const = 0;

						/**
						 * Registers an evaluation module into this play.
						 */
						void register_module(AI::HL::STP::Evaluation::Module &module);

						/**
						 * Used to update evaluation modules.
						 */
						void tick();

					protected:
						/**
						 * The World in which the Play lives.
						 */
						AI::HL::W::World &world;

						/**
						 * The constructor.
						 * You should initialize variables in the initialize() function.
						 */
						Play(AI::HL::W::World &world);

						/**
						 * Destructor
						 */
						~Play();

					private:
						/**
						 * Callback used for evaluation modules.
						 */
						sigc::signal<void> signal_tick;
				};

				/**
				 * A PlayFactory is used to construct a particular type of Play.
				 * The factory permits STP to discover all available types of Plays.
				 */
				class PlayFactory : public Registerable<PlayFactory> {
					public:
						/**
						 * Constructs a new instance of the Play corresponding to this PlayManager.
						 */
						virtual Play::Ptr create(AI::HL::W::World &world) const = 0;

						/**
						 * Constructs a new PlayFactory.
						 * Subclasses should call this constructor from their own constructors.
						 *
						 * \param[in] name a human-readable name for this Play.
						 */
						PlayFactory(const char *name);
				};

				/**
				 * An easy way to create a factory:
				 * For example:
				 * PlayFactoryImpl<GrabBall> factory_instance("Grab Ball");
				 */
				template<class P> class PlayFactoryImpl : public PlayFactory {
					public:
						PlayFactoryImpl(const char *name) : PlayFactory(name) {
						}
						Play::Ptr create(AI::HL::W::World &world) const {
							const Play::Ptr p(new P(world));
							return p;
						}
				};
			}
		}
	}
}

#endif

