#include "ai/strategy/strategy.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/move.h"
#include "ai/tactic/block.h"
#include "ai/tactic/kick.h"
#include "ai/util.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class TestPassStrategy : public Strategy {
		public:
			TestPassStrategy(World::Ptr world);
			void tick();
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const World::Ptr the_world;
	};

	TestPassStrategy::TestPassStrategy(World::Ptr world) : the_world(world) {

	}

	void TestPassStrategy::tick() {
		const FriendlyTeam &the_team(the_world->friendly);
		const Ball::Ptr the_ball(the_world->ball());

		if (the_team.size() != 2) return;

		// player 0 is the receiver
		const Player::Ptr receiver = the_team.get_player(1);
		Move move_tactic(receiver, the_world);
		move_tactic.set_position(Point(0.0, 0.0));
		move_tactic.tick();

		bool receiverhasball = AIUtil::has_ball(the_world, receiver);

		// kick it to a random place if the receiver has the ball
		if (receiverhasball) {
			std::cout << "strategy: receiver has ball, shoot randomly" << std::endl;

			Kick kick_tactic(receiver, the_world);

			srand(time(NULL));
			double randX = ((rand() % 9)-4)/2.0;
			double randY = ((rand() % 9)-4)/2.0;
		
			Point target(randX, randY);
			kick_tactic.set_target(target);
			kick_tactic.tick();
		}

		const Player::Ptr passer = the_team.get_player(0);
//		std::cout << passer->est_velocity() << std::endl;
		if (AIUtil::has_ball(the_world, passer)) {
			std::cout << "strategy: passer has ball" << std::endl;
			//Pass pass_tactic(passer, the_world, receiver);
			//pass_tactic.tick();
		} else if (receiverhasball) {
			// receiver has the ball, go somewhere?
		} else {
			std::cout << "strategy: chase ball" << std::endl;
			//Chase chase_tactic(passer, the_world);
			//chase_tactic.tick();
		}
		Pass pass_tactic(passer, the_world, receiver);
		pass_tactic.tick();
	}

	Gtk::Widget *TestPassStrategy::get_ui_controls() {
		return 0;
	}

	class TestPassStrategyFactory : public StrategyFactory {
		public:
			TestPassStrategyFactory();
			Strategy::Ptr create_strategy(World::Ptr world);
	};

	TestPassStrategyFactory::TestPassStrategyFactory() : StrategyFactory("Test(Pass) Strategy") {
	}

	Strategy::Ptr TestPassStrategyFactory::create_strategy(World::Ptr world) {
		Strategy::Ptr s(new TestPassStrategy(world));
		return s;
	}

	TestPassStrategyFactory factory;

	StrategyFactory &TestPassStrategy::get_factory() {
		return factory;
	}
}

