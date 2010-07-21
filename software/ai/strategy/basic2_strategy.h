#ifndef AI_STRATEGY_BASIC2_STRATEGY_H
#define AI_STRATEGY_BASIC2_STRATEGY_H

#include "ai/world/playtype.h"
#include "ai/strategy/strategy.h"
#include "ai/role/role.h"
#include "ai/role/defensive3.h"
#include "ai/role/offensive.h"

#include <vector>
#include <set>

/**
 * A really basic Strategy that satisfies the rule.
 * Most strategies should derive this function and overide in_play_assignment().
 * Assumes that the first robot is always the goalie.
 */
class Basic2 : public Strategy {
	public:
		Basic2(World::Ptr world);

		void tick();

		StrategyFactory &get_factory();
		Gtk::Widget *get_ui_controls();

	protected:

		void player_added(unsigned int index, Player::Ptr player);

		void player_removed(unsigned int index, Player::Ptr player);

		void playtype_changed();

		/**
		 * WARNING! Use only for
		 * - players go missing
		 * - transition into special playtype
		 *
		 * DO NOT USE FOR
		 * - during normal play
		 * - transition into normal play from freekick/kickoff/force start
		 */
		void reset_assignments();

		/**
		 * Returns the number of offenders we need.
		 * The executer of freekick/kickoff etc is an offender.
		 */
		int calc_num_offenders() const;

		const World::Ptr world;

		/// needed to keep track of state transitions
		PlayType::PlayType prev_playtype;

		/// always keep track of offenders and defenders
		std::set<Player::Ptr > defenders, offenders;
		Offensive offensive;
		Defensive3 defensive;
		Player::Ptr goalie;

		/// used for special plays
		Player::Ptr executor;
		Role::Ptr executor_role;
};

#endif
