#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H

#include "ai/world/world.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <cairomm/cairomm.h>
#include <glibmm.h>

namespace Gtk {
	class Widget;
}
class StrategyFactory;

namespace AIStrategy {
	/**
	 * Comparison for candidacy of being a goalie
	 */
	class CmpPlayerGoalie {
		public:
		bool operator()(const Player::ptr& a, const Player::ptr& b) const {
			if (a->chicker_ready_time() == Player::CHICKER_FOREVER) {
				if (b->chicker_ready_time() != Player::CHICKER_FOREVER) return true;
				return a->name < b->name;
			}
			if (b->chicker_ready_time() == Player::CHICKER_FOREVER) return false;
			return a->name < b->name;
		}
	};

	/**
	 * Sorts by chicker readiness.
	 */
	class CmpPlayerChicker {
		public:
			bool operator()(const Player::ptr& a, const Player::ptr& b) const {
				if (a->chicker_ready_time() == b->chicker_ready_time()) return a->name < b->name;
				return a->name < b->name;
			}
	};
}

/**
 * A Strategy manages the overall operation of a team. Individual AI
 * implementations should extend this class (or its subclass \c Strategy) to
 * provide their own strategy.
 */
class Strategy2 : public ByRef, public sigc::trackable {
	public:
		/**
		 * A pointer to a Strategy.
		 */
		typedef RefPtr<Strategy2> ptr;

		/**
		 * Runs the Strategy for one time tick. It is expected that the Strategy
		 * will examine the team for which it is responsible, determine if any
		 * changes need to be made to the roles or the assignments of robots to
		 * roles, make those changes (by means of Role::set_players()), and then
		 * call Role::tick() for each subsidiary Role.
		 *
		 * \param[in] overlay a Cairo context that can be drawn to in order to
		 * create an overlay graphic on the visualizer, which may be a null
		 * pointer if the visualizer is not displayed.
		 */
		virtual void tick(Cairo::RefPtr<Cairo::Context> overlay) = 0;

		/**
		 * \return the factory that creates this Strategy.
		 */
		virtual StrategyFactory &get_factory() = 0;

		/**
		 * \return the custom UI controls to manage this Strategy, or a null
		 * pointer if it does not wish to display any controls.
		 */
		virtual Gtk::Widget *get_ui_controls() = 0;
};

/**
 * A compatibility shim for strategies that do not present a visual overlay.
 */
class Strategy : public Strategy2 {
	public:
		/**
		 * Runs the Strategy for one time tick. It is expected that the Strategy
		 * will examine the team for which it is responsible, determine if any
		 * changes need to be made to the roles or the assignments of robots to
		 * roles, make those changes (by means of Role::set_players()), and then
		 * call Role::tick() for each subsidiary Role.
		 */
		virtual void tick() = 0;

	private:
		void tick(Cairo::RefPtr<Cairo::Context>) {
			tick();
		}
};

/**
 * A factory for creating Strategy objects. An individual AI implementation should
 * extend this class to provide an object which can constructs its "Strategy"
 * objects.
 */
class StrategyFactory : public Registerable<StrategyFactory> {
	public:
		/**
		 * Constructs a new Strategy.
		 *
		 * \param World the World
		 *
		 * \return The new Strategy
		 */
		virtual Strategy::ptr create_strategy(World::ptr world) = 0;

	protected:
		/**
		 * Constructs a StrategyFactory. This should be invoked from the
		 * subclass constructor when an instance of the subclass is constructed
		 * at application startup by declaring a global variable of the
		 * subclass.
		 *
		 * \param name a human-readable name for the Strategy
		 */
		StrategyFactory(const Glib::ustring &name) : Registerable<StrategyFactory>(name) {
		}
};

#endif

