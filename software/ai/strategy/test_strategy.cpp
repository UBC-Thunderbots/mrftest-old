#include "ai/strategy/strategy.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/move.h"
#include "ai/tactic/block.h"
#include "ai/tactic/move_between.h"
#include <iostream>

namespace {
	class test_strategy : public strategy {
		public:
			test_strategy(world::ptr world);
			void tick();
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const world::ptr the_world;
	};

	test_strategy::test_strategy(world::ptr world) : the_world(world) {

	}

	bool left=true;

	void test_strategy::tick() {
		const friendly_team &the_team(the_world->friendly);
		const ball::ptr the_ball(the_world->ball());

		if (the_team.size() < 5)	return;

		const point LEFT(-1.5, 0);
		const point RIGHT(1.5, 0);

		// player 0 patrols
		player::ptr receiver = the_team.get_player(0);
		move move_tactic(receiver, the_world);
//		std::cout << receiver->position() << std::endl;

		// player 1 tries to block player 0
		player::ptr blocker = the_team.get_player(1);
		block block_tactic(blocker, the_world);
		block_tactic.set_target(receiver);
		block_tactic.tick();

		if (left) {
			move_tactic.set_position(LEFT);			
			if ((receiver->position() - LEFT).lensq() < 0.05) {
				left = false;
				move_tactic.set_position(RIGHT);			
			}
		} else {
			move_tactic.set_position(RIGHT);
			if ((receiver->position() - RIGHT).lensq() < 0.05) {
				left = true;
				move_tactic.set_position(LEFT);
			}
		}
		move_tactic.tick();
		
		// player #2 stands at a particular spot
		const point STAND(-1,1);
		player::ptr passer = the_team.get_player(2);
//		std::cout << passer->position() << std::endl;
		if ((passer->position() - STAND).lensq() > 0.05) {
			move m_tactic(passer, the_world);
			m_tactic.set_position(STAND);
			m_tactic.tick();
		} else {
			pass pass_tactic(passer, the_world, receiver);
			pass_tactic.tick();
		}

		// player #3 tries to move between player #0 and player #2
		player::ptr interceptor = the_team.get_player(3);
		move_between move_between_tactic(interceptor, the_world);
		move_between_tactic.set_points(passer->position(), receiver->position());
		move_between_tactic.tick();

		// the rest of the players try to pass to player 0
		for (unsigned int i = 4; i < the_team.size(); i++)
		{
			player::ptr the_player = the_team.get_player(i);

			pass pass_tactic(the_player, the_world, receiver);
#warning has_ball
			if (the_player->sense_ball())
				std::cout << "Player " << i << " sense the ball." << std::endl;
			pass_tactic.tick();
		}	
	}

	Gtk::Widget *test_strategy::get_ui_controls() {
		return 0;
	}

	class test_strategy_factory : public strategy_factory {
		public:
			test_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	test_strategy_factory::test_strategy_factory() : strategy_factory("Test(Tactics) Strategy") {
	}

	strategy::ptr test_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new test_strategy(world));
		return s;
	}

	test_strategy_factory factory;

	strategy_factory &test_strategy::get_factory() {
		return factory;
	}
}

