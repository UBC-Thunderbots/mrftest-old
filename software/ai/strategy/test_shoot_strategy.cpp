#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/shoot.h"
#include <iostream>

namespace {
	class test_shoot_strategy : public strategy {
		public:
			test_shoot_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, player::ptr r);
		private:
	};

	test_shoot_strategy::test_shoot_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {

	}

	void test_shoot_strategy::tick() {

		player::ptr shooter = the_team->get_player(0);
		shoot::ptr shoot_tactic(new shoot(the_ball, the_field, the_team, shooter));
		
		shoot_tactic->tick();	
	}

	void test_shoot_strategy::set_playtype(playtype::playtype) {
	}
	
	Gtk::Widget *test_shoot_strategy::get_ui_controls() {
		return 0;
	}

	void test_shoot_strategy::robot_added(void){
	}

	void test_shoot_strategy::robot_removed(unsigned int index, player::ptr r){
	}

	class test_shoot_strategy_factory : public strategy_factory {
		public:
			test_shoot_strategy_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	test_shoot_strategy_factory::test_shoot_strategy_factory() : strategy_factory("Test(Shoot) Strategy") {
	}

	strategy::ptr test_shoot_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new test_shoot_strategy(ball, field, team));
		return s;
	}

	test_shoot_strategy_factory factory;

	strategy_factory &test_shoot_strategy::get_factory() {
		return factory;
	}
}

