#include "ai/strategy.h"

#include <vector> // I like vectors
using namespace std;

//this currently is just a big hack from chase_strategy, need some roles to exist before more implementations

namespace {
	class offensive_strategy : public virtual strategy {
		public:
			offensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			virtual void update();
			virtual void set_playtype(playtype::playtype t);
			virtual strategy_factory &get_factory();
			virtual Gtk::Widget *get_ui_controls();

		private:
			playtype::playtype current_playtype;

			// Create variables here (e.g. to store the roles).

			vector<role::ptr> roles;
			
	};

	offensive_strategy::offensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
		// Initialize variables here (e.g. create the roles).
		
		// offender and defender doesn't exist yet >"< 
		// role::offender off = role::offender(ball, field, team);
		// roles.push_back(off);     			  
		// role::defender def = role::defender(ball, field, team);
		// roles.push_back(def);
		
	}

	void offensive_strategy::update() {
		// Use the variables "ball", "field", and "team" to allocate players to roles.
		
		// calls role::update

		// ultimate offensive strategy: all 5 players switch to offenders (including goalie, mwahahahahahaha) 

	}

	void offensive_strategy::set_playtype(playtype::playtype t) {
		current_playtype = t;
	}
	
	Gtk::Widget *offensive_strategy::get_ui_controls() {
		return 0;
	}

	class offensive_strategy_factory : public virtual strategy_factory {
		public:
			offensive_strategy_factory();
			virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	offensive_strategy_factory::offensive_strategy_factory() : strategy_factory("offensive Strategy") {
	}

	strategy::ptr offensive_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new offensive_strategy(ball, field, team));
		return s;
	}

	offensive_strategy_factory factory;

	strategy_factory &offensive_strategy::get_factory() {
		return factory;
	}
}

