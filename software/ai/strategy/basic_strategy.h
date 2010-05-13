#ifndef AI_STRATEGY_BASIC_STRATEGY_H
#define AI_STRATEGY_BASIC_STRATEGY_H

#include "ai/strategy.h"
#include "ai/role.h"

#include <vector>

class basic_strategy : public strategy {
	public:
		basic_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
		void tick();
		void set_playtype(playtype::playtype t);
		void robot_added();
		void robot_removed(unsigned int, player::ptr);

		strategy_factory &get_factory();
		Gtk::Widget *get_ui_controls();

	protected:
		void reset_all();

		/// Assign players when playtype is play.
		virtual void in_play_assignment();

		// We need this because we don't want to make frequent changes
		static const int WAIT_AT_LEAST_TURN = 5;
		int turn_since_last_update;

		std::vector<role::ptr> roles;
};

#endif

