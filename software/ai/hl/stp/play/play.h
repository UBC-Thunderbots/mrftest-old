#pragma once

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/noncopyable.h"
#include "util/registerable.h"
#include "util/param.h"
#include <memory>
#include <glibmm/ustring.h>
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Play {
				namespace Global {
					const std::vector<std::pair<Player, const AI::HL::STP::Tactic::Tactic*>>& get_tactic_assignment();
				}

				class PlayFactory;

				/**
				 * \brief A play is a level in the STP paradigm.
				 *
				 * The purpose of a play is to execute tactics.
				 *
				 * Note that there is only one instance per class.
				 * Plays are reused, destroyed only when the AI ends.
				 * Doing so will simplify coding.
				 */
				class Play : public NonCopyable {
					public:
						using coroutine_t = boost::coroutines::coroutine<void()>;
						using caller_t = coroutine_t::caller_type;

						/**
						 * \brief Destroys the Play.
						 */
						virtual ~Play();

						/**
						 * \brief Ticks the play and yields to coroutines.
						 */
						void tick(const std::vector<bool>& players_enabled);

						/**
						 * \brief whether the coroutine has finished execution
						 */
						bool coroutine_finished() const;

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
						 * \brief A reference to this play's factory.
						 */
						virtual const PlayFactory &factory() const = 0;

						/**
						 * \brief A play can stuff on the screen if it wants to.
						 */
						virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;

						/**
						 * \brief Returns the tactic list.
						 */
						const std::array<AI::HL::STP::Tactic::Tactic::Ptr, TEAM_MAX_SIZE>& get_tactics() const;

						/**
						 * \brief Returns the current assignment.
						 */
						const std::array<AI::HL::W::Player, TEAM_MAX_SIZE>& get_assignment() const;
					protected:
						static void wait(caller_t& ca, AI::HL::STP::Tactic::Tactic* tactic);
						static void yield(caller_t& ca);

						/**
						 * \brief The World in which the Play lives.
						 */
						World world;

						/**
						 * Execute the play from a coroutine context.
						 */
						virtual void execute(caller_t& caller) = 0;

						/**
						 * \brief The constructor.
						 */
						explicit Play(World world);

						// current tactic assignment
						std::array<AI::HL::STP::Tactic::Tactic::Ptr, TEAM_MAX_SIZE> tactics;
						
						// current player assignment
						std::array<AI::HL::W::Player, TEAM_MAX_SIZE> curr_assignment;

						// previous player assignment
						std::array<AI::HL::W::Player, TEAM_MAX_SIZE> prev_assignment;

						// current player assignment flags
						std::array<AI::Flags::MoveFlags, TEAM_MAX_SIZE> assignment_flags;
					private:

						/**
						 * @brief Assigns roles to players.
						 * @return whether assignment was successful
						 */
						bool role_assignment(std::vector<bool> players_enabled);

						// the coroutine under execution
						coroutine_t _coroutine;
				};

				/**
				 * \brief A PlayFactory is used to construct a particular type of Play.
				 *
				 * The factory permits STP to discover all available types of Plays.
				 */
				class PlayFactory : public Registerable<PlayFactory> {
					public:
						/**
						 * \brief A condition that must hold ALL THE TIME, for this play to be considered and run.
						 *
						 * This is the ideal place to put conditions about playtype and minimum team size.
						 */
						virtual bool invariant(World world) const = 0;

						/**
						 * \brief For a play to be considered applicable() and invariant() must be true.
						 *
						 * This is only used once; after a play runs, applicable() is no longer called.
						 */
						virtual bool applicable(World world) const = 0;

						/**
						 * \brief Constructs a new instance of the Play corresponding to this PlayFactory.
						 */
						virtual std::unique_ptr<Play> create(World world) const = 0;

						/**
						 * \brief Constructs a new PlayFactory.
						 *
						 * Subclasses should call this constructor from their own constructors.
						 *
						 * \param[in] name a human-readable name for this Play, which must be encoded in UTF-8
						 */
						explicit PlayFactory(const char *name);

						/** 
						 * \brief Used to enable and disable a play. 
						 */ 
						BoolParam enable;

						/** 
						 * \brief Used to specify the priority / weight of a play. 
						 */ 
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

						/** 
						 * \brief Used to set a play to have static or dynamic role assignments. 
						 */ 
						BoolParam static_play;
				};

				/**
				 * \brief An easy way to create a factory.
				 *
				 * For example:
				 * PlayFactoryImpl<GrabBall> factory_instance(u8"Grab Ball");
				 */
				template<class P> class PlayFactoryImpl : public PlayFactory {
					public:
						explicit PlayFactoryImpl(const char *name) : PlayFactory(name) {
						}

						std::unique_ptr<Play> create(World world) const override {
							std::unique_ptr<Play> p(new P(world));
							return p;
						}
				};
			}
		}
	}
}
