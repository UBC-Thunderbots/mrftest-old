#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/move.h"
#include "ai/tactic/block.h"
#include "ai/tactic/move_between_robots.h"
#include <iostream>

namespace {
	class test_strategy : public strategy {
		public:
			test_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, player::ptr r);
		private:
	};

	test_strategy::test_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {

	}

	bool left=true;

	void test_strategy::tick() {

		if (the_team->size() < 5)	return;

		const point LEFT(-1.5, 0);
		const point RIGHT(1.5, 0);

		// player 0 patrols
		player::ptr receiver = the_team->get_player(0);
		move::ptr move_tactic(new move(the_ball, the_field, the_team, receiver));
//		std::cout << receiver->position() << std::endl;

		// player 1 tries to block player 0
		player::ptr blocker = the_team->get_player(1);
		block::ptr block_tactic(new block(the_ball, the_field, the_team, blocker));
		block_tactic->set_target(receiver);
		block_tactic->tick();

		if (left) {
			move_tactic->set_position(LEFT);			
			if ((receiver->position() - LEFT).lensq() < 0.05) {
				left = false;
				move_tactic->set_position(RIGHT);			
			}
		} else {
			move_tactic->set_position(RIGHT);
			if ((receiver->position() - RIGHT).lensq() < 0.05) {
				left = true;
				move_tactic->set_position(LEFT);
			}
		}
		move_tactic->tick();
		
		// player #2 stands at a particular spot
		const point STAND(-1,1);
		player::ptr passer = the_team->get_player(2);
//		std::cout << passer->position() << std::endl;
		if ((passer->position() - STAND).lensq() > 0.05) {
			move::ptr m_tactic(new move(the_ball, the_field, the_team, passer));
			m_tactic->set_position(STAND);
			m_tactic->tick();
		} else {
			pass::ptr pass_tactic (new pass(the_ball, the_field, the_team, passer));
			pass_tactic->set_receiver(receiver);
			pass_tactic->tick();				
		}

		// player #3 tries to move between player #0 and player #2
		player::ptr interceptor = the_team->get_player(3);
		move_between_robots::ptr move_between_tactic(new move_between_robots(the_ball, the_field, the_team, interceptor));
		move_between_tactic->set_robots(passer, receiver);
		move_between_tactic->tick();

		// the rest of the players try to pass to player 0
		for (unsigned int i = 4; i < the_team->size(); i++)
		{
			player::ptr the_player = the_team->get_player(i);

			pass::ptr pass_tactic (new pass(the_ball, the_field, the_team, the_player));
			pass_tactic->set_receiver(receiver);
			if (the_player->has_ball())
				std::cout << "Player " << i << " has the ball." << std::endl;
			pass_tactic->tick();					
		}	
	}

	void test_strategy::set_playtype(playtype::playtype) {
	}
	
	Gtk::Widget *test_strategy::get_ui_controls() {
		return 0;
	}

	void test_strategy::robot_added(void){
	}

	void test_strategy::robot_removed(unsigned int index, player::ptr r){
	}

	class test_strategy_factory : public strategy_factory {
		public:
			test_strategy_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	test_strategy_factory::test_strategy_factory() : strategy_factory("Test(Tactics) Strategy") {
	}

	strategy::ptr test_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new test_strategy(ball, field, team));
		return s;
	}

	test_strategy_factory factory;

	strategy_factory &test_strategy::get_factory() {
		return factory;
	}
}

