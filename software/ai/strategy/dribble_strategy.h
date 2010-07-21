#ifndef AI_STRATEGY_dribble_strategy_H
#define AI_STRATEGY_dribble_strategy_H

#include "ai/strategy/strategy.h"
#include "ai/role/role.h"

#include <vector>

/**
 * A really basic Strategy that satisfies the rule.
 * Most strategies should derive this function and overide in_play_assignment().
 * Assumes that the first robot is always the goalie.
 */
class DribbleStrategy : public Strategy {
	public:
		DribbleStrategy(World::Ptr world);
		void tick();

		StrategyFactory &get_factory();
		Gtk::Widget *get_ui_controls();

	protected:

		const World::Ptr the_world;
		int update_wait;
		int dribble_speed;
};

#endif

