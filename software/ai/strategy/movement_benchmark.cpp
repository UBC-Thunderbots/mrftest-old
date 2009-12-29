#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/move.h"
#include "gtkmm/button.h"
#include <iostream>
#include <vector>
#include <cmath>

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
			void robot_removed(unsigned int index, player::ptr r);
			void strategy_reset();
		private:
			std::vector<std::pair<point, double> > tasks;
			int time_steps;
			size_t done;
			double pos_dis_threshold;
			double pos_vel_threshold;
			double ori_dis_threshold;
			double ori_vel_threshold;
			double prev_ori;
			Gtk::Button* reset_button;
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

	movement_benchmark::movement_benchmark(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src), tasks(default_tasks, default_tasks + default_tasks_n), done(0), prev_ori(0) {
		time_steps = 0;
		done = false;
		pos_dis_threshold = 1e-1;
		pos_vel_threshold = 1e-1;
		ori_dis_threshold = 1e-1;
		ori_vel_threshold = 1e-1;
		reset_button = new Gtk::Button("Reset");
		reset_button->signal_clicked().connect(sigc::mem_fun(*this,&movement_benchmark::strategy_reset));
	}
	
	void movement_benchmark::strategy_reset() {
		done = 0;
		time_steps = 0;
	}
	
	void movement_benchmark::tick() {
		if (done < tasks.size()) {
			if (done == 0) time_steps = 0;
			else if (done > 0) time_steps++;
			const point diff_pos = the_team->get_player(0)->position() - tasks[done].first;
			const point vel_pos = the_team->get_player(0)->est_velocity();
			const double diff_ori = angle_mod(the_team->get_player(0)->orientation() - tasks[done].second);
			const double vel_ori = angle_mod(the_team->get_player(0)->orientation() - prev_ori);
			std::cout << "movement benchmark task #" << done << std::endl;
			std::cout << "displace pos:" << diff_pos.x << " " << diff_pos.y << " ori:" << diff_ori << std::endl;
			std::cout << "velocity pos:" << the_team->get_player(0)->est_velocity().x << " " << the_team->get_player(0)->est_velocity().y << " ori:" << vel_ori << std::endl;
			if (diff_pos.len() < pos_dis_threshold && vel_pos.len() < pos_vel_threshold && fabs(diff_ori) < ori_dis_threshold && fabs(vel_ori) < ori_vel_threshold) {
				std::cout << "time steps taken: " << time_steps << std::endl;
				++done;
			}
			prev_ori = the_team->get_player(0)->orientation();
		}
		the_team->get_player(0)->move(tasks[done].first, tasks[done].second);
	}

	void movement_benchmark::set_playtype(playtype::playtype) {
	}

	Gtk::Widget *movement_benchmark::get_ui_controls() {
		return reset_button;
	}

	void movement_benchmark::robot_added(void){
	}

	void movement_benchmark::robot_removed(unsigned int index, player::ptr r){
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

