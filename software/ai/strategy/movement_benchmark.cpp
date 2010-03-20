#include "ai/strategy/movement_benchmark.h"
#include <iostream>
#include <vector>
#include <cmath>

// This benchmark records how long it takes for a robot to travel and stop at a point 1 meter away.

// NOTE:
// the first task centers the robot, so the timing does not start until after that

namespace {

	class movement_benchmark_factory : public strategy_factory {
		public:
			movement_benchmark_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	movement_benchmark_factory::movement_benchmark_factory() : strategy_factory("Movement Benchmark") {
	}

	strategy::ptr movement_benchmark_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new movement_benchmark(ball, field, team));
		return s;
	}

	movement_benchmark_factory factory;

	// A set of tasks; use this for now
	const std::pair<point, double> default_tasks[] =
	{
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(1, 0), 0),
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(-1, 0), 0),
		std::make_pair(point(0, 1), 0),
		std::make_pair(point(0, -1), 0),
		std::make_pair(point(2.5, 0), 0),
		std::make_pair(point(-2.5, 0), 0),
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(0.25, 0), 0),
		std::make_pair(point(0.1, 0.1), 0),
		std::make_pair(point(-0.1, 0), 0),
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(-0.25, -0.1), 0),
		std::make_pair(point(0.25, 0.1), 0),
		std::make_pair(point(-0.1, 0), 0),
		std::make_pair(point(0, 0), 0),
	};
	const int default_tasks_n = sizeof(default_tasks) / sizeof(default_tasks[0]);

}

movement_benchmark::movement_benchmark(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team), tasks(default_tasks, default_tasks + default_tasks_n), done(0), prev_pos(0.0, 0.0), prev_ori(0) {
	time_steps = 0;
	done = tasks.size();
	pos_dis_threshold = 1e-2;
	pos_vel_threshold = 1e-2;
	ori_dis_threshold = 1e-1;
	ori_vel_threshold = 1e-1;
	reset_button = Gtk::manage(new Gtk::Button("Reset"));
	reset_button->signal_clicked().connect(sigc::mem_fun(*this,&movement_benchmark::strategy_reset));
}

void movement_benchmark::strategy_reset() {
	done = 0;
	time_steps = 0;
}

void movement_benchmark::tick() {
	if (the_team->size() == 0) {
		std::cerr << "warning: movement benchmark: nobody in the team" << std::endl;
		return;
	}
	if (done < tasks.size()) {
		if (done == 0) time_steps = 0;
		else if (done > 0) time_steps++;
		const point diff_pos = the_team->get_player(0)->position() - tasks[done].first;
		//const point vel_pos = the_team->get_player(0)->est_velocity();
		const point vel_pos = the_team->get_player(0)->position() - prev_pos;
		const double diff_ori = angle_mod(the_team->get_player(0)->orientation() - tasks[done].second);
		const double vel_ori = angle_mod(the_team->get_player(0)->orientation() - prev_ori);
		std::cout << "movement benchmark task #" << done << std::endl;
		std::cout << "displace pos:" << diff_pos.x << " " << diff_pos.y << " ori:" << diff_ori << std::endl;
		std::cout << "velocity pos:" << vel_pos.x << " " << vel_pos.y << " ori:" << vel_ori << std::endl;
		if (diff_pos.len() < pos_dis_threshold && vel_pos.len() < pos_vel_threshold && fabs(diff_ori) < ori_dis_threshold && fabs(vel_ori) < ori_vel_threshold) {
			std::cout << "time steps taken: " << time_steps << std::endl;
			++done;
		}
	}
	prev_ori = the_team->get_player(0)->orientation();
	prev_pos = the_team->get_player(0)->position();
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

strategy_factory &movement_benchmark::get_factory() {
	return factory;
}

