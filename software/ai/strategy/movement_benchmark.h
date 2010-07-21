#ifndef AI_STRATEGY_MOVEMENT_BENCHMARK_H
#define AI_STRATEGY_MOVEMENT_BENCHMARK_H

#include "ai/strategy/strategy.h"
#include "ai/tactic/move.h"
#include <gtkmm.h>
#include <ctime>

class MovementBenchmark : public Strategy {
	public:
		MovementBenchmark(World::Ptr world);
		void tick();
		void set_playtype(PlayType::PlayType t);
		StrategyFactory &get_factory();
		Gtk::Widget *get_ui_controls();
		void robot_added(void);
		void robot_removed(unsigned int index, Player::Ptr r);
		void strategy_reset();
	protected:
		const World::Ptr the_world;
		std::vector<std::pair<Point, double> > tasks;
		int time_steps;
		size_t done;
		double pos_dis_threshold;
		double pos_vel_threshold;
		double ori_dis_threshold;
		double ori_vel_threshold;
		Point prev_pos;
		double prev_ori;
		Gtk::Button reset_button;
		time_t start_tasks, curr_tasks, end_tasks;
};

#endif
