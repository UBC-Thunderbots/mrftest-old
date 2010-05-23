#include "ai/strategy/movement_benchmark.h"
#include "geom/angle.h"
#include <iostream>
#include <vector>
#include <cmath>

// This benchmark records how long it takes for a robot to travel and stop at a point 1 meter away.

// NOTE:
// the first task centers the robot, so the timing does not start until after that

// #define NO_TUNE_ROTATION
// #define TUNE_FULL
#define TUNE_HALF

namespace {

	class movement_benchmark_factory : public strategy_factory {
		public:
			movement_benchmark_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	movement_benchmark_factory::movement_benchmark_factory() : strategy_factory("Movement Benchmark (Obselete)") {
	}

	strategy::ptr movement_benchmark_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new movement_benchmark(world));
		return s;
	}

	movement_benchmark_factory factory;

	const double PI = M_PI;

#ifdef TUNE_HALF
	const std::pair<point, double> default_tasks[] =
	{
		std::make_pair(point(1.2, 0), 0),
		std::make_pair(point(0.5, 0), PI),
		std::make_pair(point(2.5, 0), 0),
		std::make_pair(point(0.5, 1.2), PI),
		std::make_pair(point(1, -0.6), 0),
		std::make_pair(point(2, 0.6), PI/2),
		std::make_pair(point(1, -0.6), -PI/2),
		std::make_pair(point(0.5, 0), 0),
		std::make_pair(point(2.5, 0.6), -PI/2),
		std::make_pair(point(1.2, 0), 0),
	};
#endif

#ifdef TUNE_FULL
	const std::pair<point, double> default_tasks[] =
	{
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(1, 0), PI),
		std::make_pair(point(-2.5, 0), 0),
		std::make_pair(point(2.5, 0), PI),
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(-1, 0), PI/2),
		std::make_pair(point(0, 1), -PI/2),
		std::make_pair(point(0, -1), 0),
		std::make_pair(point(2.5, 0), PI),
		std::make_pair(point(-2.5, 0), -PI/2),
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(0.25, 0), PI/2),
		std::make_pair(point(0.1, 0.1), 0),
		std::make_pair(point(-0.1, 0), PI),
		std::make_pair(point(0, 0), PI),
		std::make_pair(point(-0.25, -0.1), 0),
		std::make_pair(point(0.25, 0.1), PI/2),
		std::make_pair(point(-0.1, 0), 0),
		std::make_pair(point(0, 0), 0),
	};
#endif

#ifdef NO_TUNE_ROTATION
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
#endif
	const int default_tasks_n = sizeof(default_tasks) / sizeof(default_tasks[0]);

}

movement_benchmark::movement_benchmark(world::ptr world) : the_world(world), tasks(default_tasks, default_tasks + default_tasks_n), done(0), prev_pos(0.0, 0.0), prev_ori(0), reset_button("Reset") {
	time_steps = 0;
	done = tasks.size();
	pos_dis_threshold = 1e-1;
	pos_vel_threshold = 1e-1;
	ori_dis_threshold = 1e-1;
	ori_vel_threshold = 1e-1;
	reset_button.signal_clicked().connect(sigc::mem_fun(this,&movement_benchmark::strategy_reset));
}

void movement_benchmark::strategy_reset() {
	done = 0;
	time_steps = 0;
}

void movement_benchmark::tick() {
	const friendly_team &the_team(the_world->friendly);
	if (the_team.size() != 1) {
		std::cerr << "error: must have only 1 robot in the team!" << std::endl;
		return;
	}
	if (done >= tasks.size()) return;
	if (done == 0) {
		time_steps = 0;
		time(&start_tasks);
	} else if (done > 0) time_steps++;
	const point diff_pos = the_team.get_player(0)->position() - tasks[done].first;
	//const point vel_pos = the_team.get_player(0)->est_velocity();
	const point vel_pos = the_team.get_player(0)->position() - prev_pos;
	const double diff_ori = angle_mod(the_team.get_player(0)->orientation() - tasks[done].second);
	const double vel_ori = angle_mod(the_team.get_player(0)->orientation() - prev_ori);
	// std::cout << "movement benchmark task #" << done << std::endl;
	// std::cout << "displace pos:" << diff_pos.x << " " << diff_pos.y << " ori:" << diff_ori << std::endl;
	// std::cout << "velocity pos:" << vel_pos.x << " " << vel_pos.y << " ori:" << vel_ori << std::endl;
	if (diff_pos.len() < pos_dis_threshold && vel_pos.len() < pos_vel_threshold && fabs(diff_ori) < ori_dis_threshold && fabs(vel_ori) < ori_vel_threshold) {
		++done;
	}
	time(&curr_tasks);
	if (done >= tasks.size()) {
		time(&end_tasks);
		double diff = difftime(end_tasks, start_tasks);
		// task completed
		std::cout << "time steps taken: " << time_steps << " time taken=" << diff << std::endl;
		return;
	}
	prev_ori = the_team.get_player(0)->orientation();
	prev_pos = the_team.get_player(0)->position();
	the_team.get_player(0)->move(tasks[done].first, tasks[done].second);
}

void movement_benchmark::set_playtype(playtype::playtype) {
}

Gtk::Widget *movement_benchmark::get_ui_controls() {
	return &reset_button;
}

void movement_benchmark::robot_removed(unsigned int index, player::ptr r){
}

strategy_factory &movement_benchmark::get_factory() {
	return factory;
}

