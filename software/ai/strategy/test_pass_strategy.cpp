#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/move.h"
#include "ai/tactic/block.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/move_between_robots.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class test_pass_strategy : public strategy {
		public:
			test_pass_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, player::ptr r);
		private:
	};

	test_pass_strategy::test_pass_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {

	}

	void test_pass_strategy::tick() {

		if (the_team->size() != 2)	return;

		const point LEFT(-1, 0);

		// player 0 is the receiver
		player::ptr receiver = the_team->get_player(0);
		move::ptr move_tactic( new move(the_ball, the_field, the_team, receiver));
		if ((receiver->position() - LEFT).lensq() > 0.05) 
			move_tactic->set_position(LEFT);
		else 
			move_tactic->set_position(receiver->position());

		move_tactic->tick();

		// kick it to a random place if the receiver has the ball
		if (receiver->has_ball()) {

			kick::ptr kick_tactic(new kick(the_ball, the_field, the_team, receiver));

			srand(time(NULL));
			double randX = ((rand() % 9)-4)/2.0;
			double randY = ((rand() % 9)-4)/2.0;
		
			point target(randX, randY);
			kick_tactic->set_target(target);
			kick_tactic->tick();
		}

		player::ptr passer = the_team->get_player(1);
//		std::cout << passer->est_velocity() << std::endl;
		if (passer->has_ball()) {
//			std::cout << "passer has ball" << std::endl;
			pass::ptr pass_tactic( new pass(the_ball, the_field, the_team, passer));
			pass_tactic->set_receiver(receiver);
			pass_tactic->tick();
		} else {
			chase::ptr chase_tactic( new chase(the_ball, the_field, the_team, passer));
			chase_tactic->tick();
		}
	}

	void test_pass_strategy::set_playtype(playtype::playtype) {
	}
	
	Gtk::Widget *test_pass_strategy::get_ui_controls() {
		return 0;
	}

	void test_pass_strategy::robot_added(void){
	}

	void test_pass_strategy::robot_removed(unsigned int index, player::ptr r){
	}

	class test_pass_strategy_factory : public strategy_factory {
		public:
			test_pass_strategy_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	test_pass_strategy_factory::test_pass_strategy_factory() : strategy_factory("Test(Pass) Strategy") {
	}

	strategy::ptr test_pass_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new test_pass_strategy(ball, field, team));
		return s;
	}

	test_pass_strategy_factory factory;

	strategy_factory &test_pass_strategy::get_factory() {
		return factory;
	}
}

