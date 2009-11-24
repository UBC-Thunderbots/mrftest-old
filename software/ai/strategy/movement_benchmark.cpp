#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/move.h"
#include <iostream>
#include <vector>

// This benchmark records how long it takes for a robot to travel and stop at a point 1 meter away.

// NOTE:
// the first task centers the robot, so the timing does not start until after that

namespace {
	class movement_benchmark : public strategy {
		public:
			movement_benchmark(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, robot::ptr r);
		private:
			std::vector<std::pair<point, double> > tasks;
			int time_steps;
			size_t done;
			double dis_threshold;
			double vel_threshold;
			double ori_threshold;
	};

	// A set of tasks; use this for now
	const std::pair<point, double> default_tasks[] =
	{
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(1, 0), PI / 2),
		std::make_pair(point(0, 0), PI),
		std::make_pair(point(-1, 0), 0),
		std::make_pair(point(0, 1), PI / 2),
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(0, -1), PI),
		std::make_pair(point(0, 0), 0),
	};
	const int default_tasks_n = sizeof(default_tasks) / sizeof(default_tasks[0]);

	movement_benchmark::movement_benchmark(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src), tasks(default_tasks, default_tasks + default_tasks_n), done(0) {
		time_steps = 0;
		done = false;
		dis_threshold = 1e-1;
		vel_threshold = 1e-2;
		ori_threshold = 1e-1;
	}

	void movement_benchmark::tick() {
		if (done < tasks.size()) {
			if (done > 0) time_steps++;
			point diff_pos = the_team->get_player(0)->position() - tasks[done].first;
			double diff_ori = angle_mod(the_team->get_player(0)->orientation() - tasks[done].second);
			point vel_pos = the_team->get_player(0)->est_velocity();
			std::cout << "movement benchmark task #" << done << std::endl;
			std::cout << "difference pos:" << diff_pos.x << " " << diff_pos.y << " ori:" << diff_ori << std::endl;
			std::cout << "velocity: " << the_team->get_player(0)->est_velocity().x << " " << the_team->get_player(0)->est_velocity().y << std::endl;
			if (diff_pos.len() < dis_threshold && vel_pos.len() < vel_threshold && abs(diff_ori) < ori_threshold) {
				std::cout << "time steps taken: " << time_steps << std::endl;
				++done;
			}
		}
		the_team->get_player(0)->move(tasks[done].first, tasks[done].second);
	}

	void movement_benchmark::set_playtype(playtype::playtype) {
	}

	Gtk::Widget *movement_benchmark::get_ui_controls() {
		return 0;
	}

	void movement_benchmark::robot_added(void){
	}

	void movement_benchmark::robot_removed(unsigned int index, robot::ptr r){
	}

	class movement_benchmark_factory : public strategy_factory {
		public:
			movement_benchmark_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
	};

	movement_benchmark_factory::movement_benchmark_factory() : strategy_factory("Movement Benchmark") {
	}

	strategy::ptr movement_benchmark_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) {
		strategy::ptr s(new movement_benchmark(ball, field, team, pt_src));
		return s;
	}

	movement_benchmark_factory factory;

	strategy_factory &movement_benchmark::get_factory() {
		return factory;
	}
}

