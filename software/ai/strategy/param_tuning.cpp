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
	const int EVALUATION_LIMIT = 1500;

	class param_tuning : public movement_benchmark {
		public:
			param_tuning(world::ptr world);
			~param_tuning();
			Gtk::Widget *get_ui_controls();
			strategy_factory &get_factory();
			void tick();
			void reset();
			void hillclimb();
			void revert();
		private:
			const world::ptr the_world;
			Gtk::Button revert_button;
			Gtk::Button reset_button;
			Gtk::Button hillclimb_button;
			Gtk::VBox vbox;
			stochastic_local_search* sls;
			tunable_controller* tc;
			int sls_counter;
			int best;
	};

	param_tuning::param_tuning(world::ptr world) : movement_benchmark(world), the_world(world), revert_button("Redo from last best"), reset_button("Complete Reset"), hillclimb_button("Hill Climb Again"), sls(0) {
		sls_counter = 0;

		// override the reset button
		revert_button.signal_clicked().connect(sigc::mem_fun(this,&param_tuning::revert));
		reset_button.signal_clicked().connect(sigc::mem_fun(this,&param_tuning::reset));
		hillclimb_button.signal_clicked().connect(sigc::mem_fun(this,&param_tuning::hillclimb));
		best = EVALUATION_LIMIT;
		vbox.add(revert_button);
		vbox.add(reset_button);
		vbox.add(hillclimb_button);
	}

	param_tuning::~param_tuning() {
		if(sls != NULL) delete sls;
	}

	Gtk::Widget *param_tuning::get_ui_controls() {
		return &vbox;
	}

	void param_tuning::reset() {
		if (sls != NULL) delete sls;
		tc = tunable_controller::get_instance();
		if (tc == NULL) return;
		sls = new stochastic_local_search(tc->get_params_min(), tc->get_params_max());
		done = 0;
		time_steps = 0;
		best = EVALUATION_LIMIT;
		sls_counter = 0;
		tc->set_params(sls->get_best_params());
		std::cout << " reset, curr params=";
		const std::vector<double>& params = sls->get_best_params();
		for (unsigned int i = 0; i < params.size(); ++i) {
			std::cout << params[i] << " ";
		}
		std::cout << std::endl;
	}

	void param_tuning::hillclimb() {
		const std::vector<double> best_params = sls->get_best_params();
		sls->hill_climb();
		tc->set_params(sls->get_params());
		done = 0;
		time_steps = 0;
		std::cout << " hill climb, curr params=";
		const std::vector<double>& params = sls->get_params();
		for (unsigned int i = 0; i < params.size(); ++i) {
			std::cout << params[i] << " ";
		}
		std::cout << std::endl;
	}

	void param_tuning::revert() {
		sls->revert();
		tc->set_params(sls->get_best_params());
		done = 0;
		time_steps = 0;
		std::cout << " revert curr params=";
		const std::vector<double>& params = sls->get_params();
		for (unsigned int i = 0; i < params.size(); ++i) {
			std::cout << params[i] << " ";
		}
		std::cout << std::endl;
	}

	void param_tuning::tick() {
		if (tc == NULL || tc != tunable_controller::get_instance()) {
			reset();
			return;
		}
		// std::cout << " tick " << std::endl;
		if (sls_counter > TUNING_ITERATIONS) {
			// done with sls
			return;
		}
		if (the_world->friendly.size() != 1) {
			std::cerr << "error: must have only 1 robot in the team!" << std::endl;
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
			std::cout << "setting new params" << std::endl;
			std::cout << "curr params=";
			const std::vector<double>& params = sls->get_params();
			for (unsigned int i = 0; i < params.size(); ++i) {
				std::cout << params[i] << " ";
			}
			std::cout << std::endl;
			const std::vector<double> best_params = sls->get_best_params();
			std::cout << "best params=";
			for (unsigned int i = 0; i < best_params.size(); ++i) {
				std::cout << best_params[i] << " ";
			}
			std::cout << std::endl;
			sls_counter++;
			done = 0;
			time_steps = 0;
		}
		movement_benchmark::tick();
	}

	class param_tuning_factory : public strategy_factory {
		public:
			param_tuning_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	param_tuning_factory::param_tuning_factory() : strategy_factory("Param Tuning") {
	}

	strategy::ptr param_tuning_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new param_tuning(world));
		return s;
	}

	param_tuning_factory factory;

	strategy_factory &param_tuning::get_factory() {
		return factory;
	}

}
