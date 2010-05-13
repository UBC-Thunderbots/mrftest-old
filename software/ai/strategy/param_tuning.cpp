#include "ai/strategy/movement_benchmark.h"
#include "robot_controller/tunable_controller.h"
#include "util/stochastic_local_search.h"
#include <iostream>
#include <vector>
#include <cmath>

// Parameter Tuning based on movement benchmark
// To change tasks, make use of movement_benchmark

namespace {

	const int TUNING_ITERATIONS = 1000;
	const int EVALUATION_LIMIT = 1000;

	class param_tuning : public movement_benchmark {
		public:
			param_tuning(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			~param_tuning();
			strategy_factory &get_factory();
			void tick();
			void strategy_reset();
		private:
			stochastic_local_search* sls;
			tunable_controller* tc;
			int sls_counter;
			int best;
	};

	param_tuning::param_tuning(ball::ptr ball, field::ptr field, controlled_team::ptr team) : movement_benchmark(ball, field, team), sls(NULL) {
		sls_counter = 0;

		// override the reset button
		reset_button = Gtk::manage(new Gtk::Button("Reset"));
		reset_button->signal_clicked().connect(sigc::mem_fun(this,&param_tuning::strategy_reset));
		best = EVALUATION_LIMIT;
	}

	param_tuning::~param_tuning() {
		if(sls != NULL) delete sls;
	}

	void param_tuning::strategy_reset() {
		if (sls != NULL) delete sls;
		tc = tunable_controller::controller_instance;
		if (tc == NULL) return;
		sls = new stochastic_local_search(tc->get_params_min(), tc->get_params_max());
		done = 0;
		time_steps = 0;
		best = EVALUATION_LIMIT;
		sls_counter = 0;
		tc->set_params(sls->get_params());
		std::cout << " reset, curr params=";
		const std::vector<double>& params = sls->get_params();
		for (int i = 0; i < (int)params.size(); ++i) {
			std::cout << params[i] << " ";
		}
		std::cout << std::endl;
	}

	void param_tuning::tick() {
		if (tc == NULL) {
			std::cerr << "Error: Use tunable robot controller and press reset" << std::endl;
			return;
		}
		// std::cout << " tick " << std::endl;
		if (sls_counter > TUNING_ITERATIONS) {
			// done with sls
			return;
		}
		if (the_team->size() != 1) {
			std::cerr << "error: must have only 1 robot in the team!" << std::endl;
			return;
		}
		if (tc != tunable_controller::controller_instance) {
			std::cerr << "error: robot controller changed; press reset" << std::endl;
			return;
		}
		if (done >= tasks.size() || time_steps > best) {
			if (time_steps < best) {
				best = time_steps;
				sls->set_cost(time_steps);
				sls->hill_climb();
			} else {
				sls->set_cost(EVALUATION_LIMIT);
				sls->hill_climb();
			}
			tc->set_params(sls->get_params());
			std::cout << "time steps taken=" << time_steps << std::endl;
			std::cout << "setting new params" << std::endl;
			std::cout << "curr params=";
			const std::vector<double>& params = sls->get_params();
			for (int i = 0; i < (int)params.size(); ++i) {
				std::cout << params[i] << " ";
			}
			std::cout << std::endl;
			const std::vector<double>& best_params = sls->get_best_params();
			std::cout << "best params=";
			for (int i = 0; i < (int)best_params.size(); ++i) {
				std::cout << best_params[i] << " ";
			}
			std::cout << std::endl;
			sls_counter++;
			done = 0;
			time_steps = 0;
		} else {
			time_steps++;
			if (done == 0) {
				// do something
			} else if (done > 0) {
			}
			const point diff_pos = the_team->get_player(0)->position() - tasks[done].first;
			//const point vel_pos = the_team->get_player(0)->est_velocity();
			const point vel_pos = the_team->get_player(0)->position() - prev_pos;
			const double diff_ori = angle_mod(the_team->get_player(0)->orientation() - tasks[done].second);
			const double vel_ori = angle_mod(the_team->get_player(0)->orientation() - prev_ori);
			//std::cout << "movement benchmark task #" << done << " time step=" << time_steps << std::endl;
			//std::cout << "displace pos:" << diff_pos.x << " " << diff_pos.y << " ori:" << diff_ori << std::endl;
			//std::cout << "velocity pos:" << vel_pos.x << " " << vel_pos.y << " ori:" << vel_ori << std::endl;
			if (diff_pos.len() < pos_dis_threshold && vel_pos.len() < pos_vel_threshold && fabs(diff_ori) < ori_dis_threshold && fabs(vel_ori) < ori_vel_threshold) {
				//std::cout << "time steps taken: " << time_steps << std::endl;
				if(done == 0) {
					time_steps = 0;
				}
				++done;
			}
		}
		prev_ori = the_team->get_player(0)->orientation();
		prev_pos = the_team->get_player(0)->position();
		the_team->get_player(0)->move(tasks[done].first, tasks[done].second);
	}

	class param_tuning_factory : public strategy_factory {
		public:
			param_tuning_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	param_tuning_factory::param_tuning_factory() : strategy_factory("Param Tuning") {
	}

	strategy::ptr param_tuning_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new param_tuning(ball, field, team));
		return s;
	}

	param_tuning_factory factory;

	strategy_factory &param_tuning::get_factory() {
		return factory;
	}

}
