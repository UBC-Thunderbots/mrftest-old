#ifndef AI_STRATEGY_BASIC_STRATEGY_H
#define AI_STRATEGY_BASIC_STRATEGY_H

#include "ai/strategy/strategy.h"
#include "ai/role/role.h"

#include <vector>

/**
 * A really basic Strategy that satisfies the rule.
 * Most strategies should derive this function and overide in_play_assignment().
 * Assumes that the first robot is always the goalie.
 */
class BasicStrategy : public Strategy {
	public:
		BasicStrategy(RefPtr<World> world);
		void tick();

		StrategyFactory &get_factory();
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
		 * Assign players when playtype is play.
		 * This can be overriden for more complex intelligence.
		 * If there is no player, returns a null refpointer and do nothing.
		 * If there is only one player, returns that player.
		 * Otherwise,
		 * Assigns players as per normal playtype.
		 * But leave out one offender, so that it can be used for other things like
		 * free kicks.
		 */
		virtual RefPtr<Player> minus_one_assignment();

		/**
		 * Amount of ticks per update.
		 * To save processing time, we only update if necessary.
		 * By default, the BasicStrategy will update every 5 turns.
		 * This is used only when playtype is play.
		 */
		int update_wait_turns;

		/**
		 * Robot roles.
		 */
		std::vector<RefPtr<Role> > roles;

		const RefPtr<World> the_world;
		int update_wait;
};

#endif

