#ifndef AI_HL_STP_PLAY_PLAY_H
#define AI_HL_STP_PLAY_PLAY_H

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/noncopyable.h"
#include "util/registerable.h"
#include "util/param.h"
#include <memory>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Play {
				class PlayFactory;

				/**
				 * \brief A play is a level in the STP paradigm.
				 *
				 * The purpose of a play is to produce roles.
				 * A role is a sequence of tactic.
				 *
				 * Note that there is only one instance per class.
				 * Plays are reused, destroyed only when the AI ends.
				 * Doing so will simplify coding.
				 *
				 * Please see the instructions in the assign() function.
				 */
				class Play : public NonCopyable {
					public:
						/**
						 * \brief Destroys the Play.
						 */
						virtual ~Play();

						/**
						 * \brief A condition that must hold ALL THE TIME, for this play to be considered and run.
						 *
						 * This is the ideal place to put conditions about playtype and minimum team size.
						 */
						virtual bool invariant() const = 0;

						/**
						 * \brief For a play to be considered applicable() and invariant() must be true.
						 *
						 * This is only used once; after a play runs, applicable() is no longer called.
						 */
						virtual bool applicable() const = 0;

						/**
						 * \brief Checks if this play has succeeded.
						 *
						 * A subclass must implement this function.
						 */
						virtual bool done() const = 0;

						/**
						 * \brief Checks if this play has failed.
						 *
						 * A subclass must implement this function.
						 */
						virtual bool fail() const = 0;

						/**
						 * \brief \c true if this play can give up safely right now.
						 */
						virtual bool can_give_up_safely() const;

						/**
						 * \brief Provide lists of tactics.
						 *
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
						virtual void assign(std::vector<Tactic::Tactic::Ptr> &goalie_role, std::vector<Tactic::Tactic::Ptr>(&roles)[STP::TEAM_MAX_SIZE-1]) = 0;

						/**
						 * \brief A reference to this play's factory.
						 */
						virtual const PlayFactory &factory() const = 0;

						/**
						 * \brief A play can stuff on the screen if it wants to.
						 */
						virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;

					protected:
						/**
						 * \brief The World in which the Play lives.
						 */
						World world;

						/**
						 * \brief The constructor.
						 */
						Play(World world);
				};

				/**
				 * \brief A PlayFactory is used to construct a particular type of Play.
				 *
				 * The factory permits STP to discover all available types of Plays.
				 */
				class PlayFactory : public Registerable<PlayFactory> {
					public:
						/**
						 * \brief Constructs a new instance of the Play corresponding to this PlayFactory.
						 */
						virtual std::unique_ptr<Play> create(World world) const = 0;

						/**
						 * \brief Constructs a new PlayFactory.
						 *
						 * Subclasses should call this constructor from their own constructors.
						 *
						 * \param[in] name a human-readable name for this Play.
						 */
						PlayFactory(const char *name);

						BoolParam enable;

						IntParam priority;
						
						/** 
						 * \brief Used to determine the playbook this play belongs to (use with bitmask)
						 * Playbook 1 is standard play
						 * Playbook 2 is mixed team defense
						 * Playbook 3 is mixed team offense
						 * ...
						 * If you want a particular play to be available in all playbooks, simply set it 
						 * to the maximum bitmask value (currently set to 1023 to allow a total of 10 playbooks.) 
						 */
						IntParam playbook;
				};

				/**
				 * \brief An easy way to create a factory.
				 *
				 * For example:
				 * PlayFactoryImpl<GrabBall> factory_instance("Grab Ball");
				 */
				template<class P> class PlayFactoryImpl : public PlayFactory {
					public:
						PlayFactoryImpl(const char *name) : PlayFactory(name) {
						}

						std::unique_ptr<Play> create(World world) const {
							std::unique_ptr<Play> p(new P(world));
							return p;
						}
				};
			}
		}
	}
}

#endif

