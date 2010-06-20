#ifndef AI_STRATEGY_dribble_strategy_H
#define AI_STRATEGY_dribble_strategy_H

#include "ai/strategy/strategy.h"
#include "ai/role/role.h"

#include <vector>

/**
 * A really basic strategy that satisfies the rule.
 * Most strategies should derive this function and overide in_play_assignment().
 * Assumes that the first robot is always the goalie.
 */
class dribble_strategy : public strategy {
	public:
		dribble_strategy(world::ptr world);
		void tick();

		strategy_factory &get_factory();
		Gtk::Widget *get_ui_controls();

	protected:

		const world::ptr the_world;
		int update_wait;
		int dribble_speed;
};

#endif

