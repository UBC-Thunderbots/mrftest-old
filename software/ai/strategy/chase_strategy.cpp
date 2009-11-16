#include "ai/strategy.h"
#include "ai/tactic.h"
#include "ai/tactic/chase.h"

namespace {
	class chase_strategy : public strategy {
		public:
			chase_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, robot::ptr r);
		private:
	};

	chase_strategy::chase_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src) {
	}

	void chase_strategy::tick() {
		for (unsigned int i = 0; i < the_team->size(); i++)
		{
			tactic::ptr chaser (new chase(the_ball, the_field, the_team, the_team->get_player(i)));
			chaser->tick();
		}
	}

	void chase_strategy::set_playtype(playtype::playtype) {
	}
	
	Gtk::Widget *chase_strategy::get_ui_controls() {
		return 0;
	}

	void chase_strategy::robot_added(void){
	}

	void chase_strategy::robot_removed(unsigned int index, robot::ptr r){
	}

	class chase_strategy_factory : public strategy_factory {
		public:
			chase_strategy_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
	};

	chase_strategy_factory::chase_strategy_factory() : strategy_factory("Chase Strategy") {
	}

	strategy::ptr chase_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) {
		strategy::ptr s(new chase_strategy(ball, field, team, pt_src));
		return s;
	}

	chase_strategy_factory factory;

	strategy_factory &chase_strategy::get_factory() {
		return factory;
	}
}

