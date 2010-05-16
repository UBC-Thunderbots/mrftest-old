#ifndef AI_STRATEGY_BASIC_STRATEGY_H
#define AI_STRATEGY_BASIC_STRATEGY_H

#include "ai/strategy/strategy.h"
#include "ai/role/role.h"

#include <vector>

/**
 * A really basic strategy that satisfies the rule.
 * Most strategies should derive this function and overide in_play_assignment().
 * Assumes that the first robot is always the goalie.
 */
class basic_strategy : public strategy {
	public:
		basic_strategy(world::ptr world);
		void tick();

		strategy_factory &get_factory();
		Gtk::Widget *get_ui_controls();

	protected:

		/**
		 * Reassigns all robots.
		 */
		void reset_all();

		/**
		 * Assign players when playtype is play.
		 * This can be overriden for more complex intelligence.
		 */
		virtual void in_play_assignment();

		/**
		 * Amount of ticks per update.
		 * To save processing time, we only update if necessary.
		 * By default, the basic_strategy will update every 5 turns.
		 * This is used only when playtype is play.
		 */
		int update_wait_turns;

		/**
		 * Robot roles.
		 */
		std::vector<role::ptr> roles;

	private:
		const world::ptr the_world;
		int update_wait;
};

#endif

