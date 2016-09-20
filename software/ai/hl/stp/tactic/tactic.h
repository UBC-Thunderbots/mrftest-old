#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/stp/world.h"
#include "ai/backend/primitives/all.h"
#include "util/noncopyable.h"
#include "util/registerable.h"
#include <memory>
#include <set>
#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <glibmm/ustring.h>
#include <boost/coroutine/coroutine.hpp>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * \brief A tactic is a layer in the STP paradigm.
				 *
				 * See STP paper section 4.1 and the wiki.
				 *
				 * A tactic is an action and has verb names.
				 *
				 * Every subclass must implement execute().
				 * Non-goalie tactics must implement select().
				 * Active tactics must implement done().
				 * Subclass may optionally implement player_changed().
				 *
				 * To prevent rapid fluctuation of parameters,
				 * hysteresis (thresholding) is recommended.
				 */
				class Tactic : public NonCopyable {
					public:
						using coroutine_t = boost::coroutines::coroutine<void()>;
						using caller_t = coroutine_t::caller_type;

						/**
						 * \brief A pointer to a Tactic.
						 */
						typedef std::unique_ptr<Tactic> Ptr;

						/**
						 * \brief Destroys the Tactic.
						 */
						virtual ~Tactic();

						/**
						 * \brief Returns whether the Tactic is done. By default, this returns 
						 * whether the coroutine has finished execution.
						 */
						virtual bool done() const;

						/**
						 * \brief Selects a player from the set.
						 *
						 * A non-goalie tactic must implement this function.
						 *
						 * \param[in] players a set of players to choose from
						 *
						 * \return a player to be used by this tactic
						 */
						virtual Player select(const std::set<Player> &players) const = 0;

						/**
						 * \brief Returns the player currently associated with this tactic.
						 *
						 * \return the player
						 */
						Player player() const;

						/**
						 * \brief Changes the player associated with this tactic.
						 */
						void player(Player p);

						/**
						 * \brief Transfers control to the tactic coroutine.
						 */
						void tick();

						/**
						 * \brief Returns a string description of this tactic.
						 *
						 * \return a description of the tactic.
						 */
						virtual Glib::ustring description() const = 0;

						/**
						 * \brief An optional function to draw extra stuff on the overlay.
						 */
						virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> context) const;

					protected:
						World world;

						/**
						 * \brief The main execution of this tactic.
						 *
						 * This function runs every tick.
						 * A subclass must implement this function.
						 */
						virtual void execute(caller_t& caller) = 0;

						/**
						 * \brief Constructor for tactic.
						 */
						explicit Tactic(World world);

						/**
						 * \brief Triggerred when the player associated changes.
						 */
						virtual void player_changed();

						/**
						 * \brief Yields and waits for the primitive to complete.
						 */
						static void wait(caller_t& ca, const AI::BE::Primitives::Primitive& prim);

						/**
						 * \brief Yields for one tick.
						 */
						static void yield(caller_t& ca);

					private:
						Player player_;
						coroutine_t coroutine_;
				};
			}
		}
	}
}

#endif
