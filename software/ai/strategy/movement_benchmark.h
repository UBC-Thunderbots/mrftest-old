#ifndef AI_STRATEGY_MOVEMENT_BENCHMARK_H
#define AI_STRATEGY_MOVEMENT_BENCHMARK_H

#include "ai/strategy/strategy.h"
#include "ai/tactic/move.h"
#include <gtkmm.h>

class movement_benchmark : public strategy {
	public:
		movement_benchmark(world::ptr world);
		void tick();
		void set_playtype(playtype::playtype t);
		strategy_factory &get_factory();
		Gtk::Widget *get_ui_controls();
		void robot_added(void);
		void robot_removed(unsigned int index, player::ptr r);
		void strategy_reset();
	protected:
		const world::ptr the_world;
		std::vector<std::pair<point, double> > tasks;
		int time_steps;
		size_t done;
		double pos_dis_threshold;
		double pos_vel_threshold;
		double ori_dis_threshold;
		double ori_vel_threshold;
		point prev_pos;
		double prev_ori;
		Gtk::Button reset_button;
};

#endif
