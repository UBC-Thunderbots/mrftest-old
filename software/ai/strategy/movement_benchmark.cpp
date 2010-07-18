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

	class MovementBenchmarkFactory : public StrategyFactory {
		public:
			MovementBenchmarkFactory();
			Strategy::ptr create_strategy(World::ptr world);
	};

	MovementBenchmarkFactory::MovementBenchmarkFactory() : StrategyFactory("Movement Benchmark (Obselete)") {
	}

	Strategy::ptr MovementBenchmarkFactory::create_strategy(World::ptr world) {
		Strategy::ptr s(new MovementBenchmark(world));
		return s;
	}

	MovementBenchmarkFactory factory;

	const double PI = M_PI;

#ifdef TUNE_HALF
	const std::pair<Point, double> default_tasks[] =
	{
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(0.5, 0), PI),
		std::make_pair(Point(2.5, 0), 0),
		std::make_pair(Point(0.5, 1.2), PI),
		std::make_pair(Point(1, -0.6), 0),
		/*std::make_pair(Point(2, 0.6), PI/2),
		std::make_pair(Point(1, -0.6), -PI/2),
		std::make_pair(Point(0.5, 0), 0),
		std::make_pair(Point(2.5, 0.6), -PI/2),
		std::make_pair(Point(1.2, 0), 0),*/
	};
#endif

#ifdef TUNE_FULL
	const std::pair<Point, double> default_tasks[] =
	{
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(1, 0), PI),
		std::make_pair(Point(-2.5, 0), 0),
		std::make_pair(Point(2.5, 0), PI),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(-1, 0), PI/2),
		std::make_pair(Point(0, 1), -PI/2),
		std::make_pair(Point(0, -1), 0),
		std::make_pair(Point(2.5, 0), PI),
		std::make_pair(Point(-2.5, 0), -PI/2),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(0.25, 0), PI/2),
		std::make_pair(Point(0.1, 0.1), 0),
		std::make_pair(Point(-0.1, 0), PI),
		std::make_pair(Point(0, 0), PI),
		std::make_pair(Point(-0.25, -0.1), 0),
		std::make_pair(Point(0.25, 0.1), PI/2),
		std::make_pair(Point(-0.1, 0), 0),
		std::make_pair(Point(0, 0), 0),
	};
#endif

#ifdef NO_TUNE_ROTATION
	const std::pair<Point, double> default_tasks[] =
	{
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(1, 0), 0),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(-1, 0), 0),
		std::make_pair(Point(0, 1), 0),
		std::make_pair(Point(0, -1), 0),
		std::make_pair(Point(2.5, 0), 0),
		std::make_pair(Point(-2.5, 0), 0),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(0.25, 0), 0),
		std::make_pair(Point(0.1, 0.1), 0),
		std::make_pair(Point(-0.1, 0), 0),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(-0.25, -0.1), 0),
		std::make_pair(Point(0.25, 0.1), 0),
		std::make_pair(Point(-0.1, 0), 0),
		std::make_pair(Point(0, 0), 0),
	};
#endif
	const int default_tasks_n = sizeof(default_tasks) / sizeof(default_tasks[0]);

}

MovementBenchmark::MovementBenchmark(World::ptr world) : the_world(world), tasks(default_tasks, default_tasks + default_tasks_n), done(0), prev_pos(0.0, 0.0), prev_ori(0), reset_button("Reset") {
	time_steps = 0;
	done = tasks.size();
	pos_dis_threshold = 0.2;
	pos_vel_threshold = 0.2;
	ori_dis_threshold = 0.2;
	ori_vel_threshold = 0.2;
	reset_button.signal_clicked().connect(sigc::mem_fun(this,&MovementBenchmark::strategy_reset));
}

void MovementBenchmark::strategy_reset() {
	done = 0;
	time_steps = 0;
}

void MovementBenchmark::tick() {
	const FriendlyTeam &the_team(the_world->friendly);
	if (the_team.size() != 1) {
		std::cerr << "error: must have only 1 robot in the team!" << std::endl;
		return;
	}
	if (done >= tasks.size()) return;
	if (done == 0) {
		time_steps = 0;
		time(&start_tasks);
	} else if (done > 0) time_steps++;
	const Point diff_pos = the_team.get_player(0)->position() - tasks[done].first;
	//const Point vel_pos = the_team.get_player(0)->est_velocity();
	const Point vel_pos = the_team.get_player(0)->position() - prev_pos;
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

void MovementBenchmark::set_playtype(PlayType::PlayType) {
}

Gtk::Widget *MovementBenchmark::get_ui_controls() {
	return &reset_button;
}

void MovementBenchmark::robot_removed(unsigned int index, Player::ptr r){
}

StrategyFactory &MovementBenchmark::get_factory() {
	return factory;
}

