#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H

#include "ai/world/world.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <glibmm.h>

namespace Gtk {
	class Widget;
}
class strategy_factory;

/**
 * A strategy manages the overall operation of a team. Individual AI
 * implementations should extend this class to provide their own strategy.
 */
class strategy : public byref, public sigc::trackable {
	public:
		/**
		 * A pointer to a strategy.
		 */
		typedef Glib::RefPtr<strategy> ptr;

		/**
		 * Runs the strategy for one time tick. It is expected that the strategy
		 * will examine the team for which it is responsible, determine if any
		 * changes need to be made to the roles or the assignments of robots to
		 * roles, make those changes (by means of role::set_robots()), and then
		 * call role::tick() for each subsidiary role.
		 */
		virtual void tick() = 0;

		/**
		 * \return The factory that creates this strategy
		 */
		virtual strategy_factory &get_factory() = 0;

		/**
		 * \return The custom UI controls to manage this strategy, or a null
		 * pointer if it does not wish to display any controls
		 */
		virtual Gtk::Widget *get_ui_controls() = 0;
};

/**
 * A factory for creating strategy objects. An individual AI implementation should
 * extend this class to provide an object which can constructs its "strategy"
 * objects.
 */
class strategy_factory : public registerable<strategy_factory> {
	public:
		/**
		 * Constructs a new strategy.
		 *
		 * \param world the world
		 *
		 * \return The new strategy
		 */
		virtual strategy::ptr create_strategy(world::ptr world) = 0;

	protected:
		/**
		 * Constructs a strategy_factory. This should be invoked from the
		 * subclass constructor when an instance of the subclass is constructed
		 * at application startup by declaring a global variable of the
		 * subclass.
		 *
		 * \param name a human-readable name for the strategy
		 */
		strategy_factory(const Glib::ustring &name) : registerable<strategy_factory>(name) {
		}
};

#endif

