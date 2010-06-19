#ifndef AI_STRATEGY_BASIC_STRATEGY_H
#define AI_STRATEGY_BASIC_STRATEGY_H

#include "ai/strategy/better_strategy.h"

/**
 * A really basic strategy that satisfies the rule.
 * Most strategies should derive this function and overide in_play_assignment().
 * Assumes that the first robot is always the goalie.
 */
class basic_strategy : public basic_strategy {
	public:
		basic_strategy(world::ptr world);
		void tick();

		strategy_factory &get_factory();
		Gtk::Widget *get_ui_controls();

	protected:

		void in_play_assignment();

		player::ptr minus_one_assignment();
};

#endif

