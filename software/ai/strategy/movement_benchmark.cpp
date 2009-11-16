#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/move.h"
#include "iostream"

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
			point destination;
			int time_steps;
			bool done;
			double threshold;
	};

	movement_benchmark::movement_benchmark(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src) {
		destination = team->get_player(0)->position();
		destination.x += 1;
		destination.y += 1;
		time_steps = 0;
		done = false;
		threshold = 0.1;
	}

	void movement_benchmark::tick() {
		if (!done) {
			time_steps++;
			double xDiff = the_team->get_player(0)->position().x - destination.x;
			double yDiff = the_team->get_player(0)->position().y - destination.y;
			if (xDiff < 0) xDiff *= -1;
			if (yDiff < 0) yDiff *= -1;
			std::cout << xDiff << " " << yDiff << std::endl;
			if (xDiff < threshold && yDiff < threshold) {
				std::cout << "time steps taken: " << time_steps << std::endl;
				done = true;
			}
			move::ptr mover (new move(the_ball, the_field, the_team, the_team->get_player(0)));
			mover->set_position(destination);
			mover->tick();
		}
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

