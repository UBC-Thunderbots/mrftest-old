#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/move.h"
#include "ai/tactic/block.h"


namespace {
	class test_strategy : public strategy {
		public:
			test_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, robot::ptr r);
		private:
	};

	test_strategy::test_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src) {

	}

	bool left=true;

	void test_strategy::tick() {

		if (the_team->size() < 3)	return;

		const point LEFT(-1.5, 0);
		const point RIGHT(1.5, 0);

		// player 0 patrols
		player::ptr receiver = the_team->get_player(0);
		move::ptr move_tactic(new move(the_ball, the_field, the_team, receiver));
//		std::cout << receiver->position() << std::endl;

		// player 1 tries to block player 0
//		player::ptr blocker = the_team->get_player(1);
//		block::ptr block_tactic(new block(the_ball, the_field, the_team, blocker));
//		block_tactic->set_target(receiver);
//		block_tactic->tick();

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
		
		// the rest of the players try to pass to player 0
		for (unsigned int i = 2; i < the_team->size(); i++)
		{
			player::ptr the_player = the_team->get_player(i);

			pass::ptr pass_tactic (new pass(the_ball, the_field, the_team, the_player));
			pass_tactic->set_receiver(receiver);
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

	void test_strategy::robot_removed(unsigned int index, robot::ptr r){
	}

	class test_strategy_factory : public strategy_factory {
		public:
			test_strategy_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
	};

	test_strategy_factory::test_strategy_factory() : strategy_factory("Test(Pass) Strategy") {
	}

	strategy::ptr test_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) {
		strategy::ptr s(new test_strategy(ball, field, team, pt_src));
		return s;
	}

	test_strategy_factory factory;

	strategy_factory &test_strategy::get_factory() {
		return factory;
	}
}

