#include "ai/strategy/strategy.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/move.h"
#include "ai/tactic/block.h"
#include "ai/tactic/move_between.h"
#include <iostream>

namespace {
	class TestStrategy : public Strategy {
		public:
			TestStrategy(World::ptr world);
			void tick();
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const World::ptr the_world;
	};

	TestStrategy::TestStrategy(World::ptr world) : the_world(world) {

	}

	bool left=true;

	void TestStrategy::tick() {
		const FriendlyTeam &the_team(the_world->friendly);
		const Ball::ptr the_ball(the_world->ball());

		if (the_team.size() < 5)	return;

		const Point LEFT(-1.5, 0);
		const Point RIGHT(1.5, 0);

		// player 0 patrols
		Player::ptr receiver = the_team.get_player(0);
		Move move_tactic(receiver, the_world);
//		std::cout << receiver->position() << std::endl;

		// player 1 tries to block player 0
		Player::ptr blocker = the_team.get_player(1);
		Block block_tactic(blocker, the_world);
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
		const Point STAND(-1,1);
		Player::ptr passer = the_team.get_player(2);
//		std::cout << passer->position() << std::endl;
		if ((passer->position() - STAND).lensq() > 0.05) {
			Move m_tactic(passer, the_world);
			m_tactic.set_position(STAND);
			m_tactic.tick();
		} else {
			Pass pass_tactic(passer, the_world, receiver);
			pass_tactic.tick();
		}

		// player #3 tries to move between player #0 and player #2
		Player::ptr interceptor = the_team.get_player(3);
		MoveBetween move_between_tactic(interceptor, the_world);
		move_between_tactic.set_points(passer->position(), receiver->position());
		move_between_tactic.tick();

		// the rest of the players try to pass to player 0
		for (unsigned int i = 4; i < the_team.size(); i++)
		{
			Player::ptr the_player = the_team.get_player(i);

			Pass pass_tactic(the_player, the_world, receiver);
#warning has_ball
			if (the_player->sense_ball())
				std::cout << "Player " << i << " sense the ball." << std::endl;
			pass_tactic.tick();
		}	
	}

	Gtk::Widget *TestStrategy::get_ui_controls() {
		return 0;
	}

	class TestStrategyFactory : public StrategyFactory {
		public:
			TestStrategyFactory();
			Strategy::ptr create_strategy(World::ptr world);
	};

	TestStrategyFactory::TestStrategyFactory() : StrategyFactory("Test(Tactics) Strategy") {
	}

	Strategy::ptr TestStrategyFactory::create_strategy(World::ptr world) {
		Strategy::ptr s(new TestStrategy(world));
		return s;
	}

	TestStrategyFactory factory;

	StrategyFactory &TestStrategy::get_factory() {
		return factory;
	}
}

