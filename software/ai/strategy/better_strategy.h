#ifndef AI_STRATEGY_BETTER_STRATEGY_H
#define AI_STRATEGY_BETTER_STRATEGY_H

#include "ai/strategy/basic_strategy.h"

/**
 * A really basic strategy that satisfies the rule.
 * Most strategies should derive this function and overide in_play_assignment().
 * Assumes that the first robot is always the goalie.
 */
class better_strategy : public basic_strategy {
	public:
		better_strategy(world::ptr w);

		strategy_factory &get_factory();
		Gtk::Widget *get_ui_controls();

	protected:

		void in_play_assignment();

		player::ptr minus_one_assignment();
};

#endif

