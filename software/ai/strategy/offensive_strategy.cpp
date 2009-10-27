#include "ai/strategy.h"
#include <algorithm>
#include <vector> // I like vectors, like it or die

// made by Terence under braindead conditions with a crazy mind
using namespace std;

namespace {
	class offensive_strategy : public virtual strategy {
		public:
			offensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			virtual void update();
			virtual void set_playtype(playtype::playtype t);
			virtual strategy_factory &get_factory();
			virtual Gtk::Widget *get_ui_controls();
      virtual void handleRobotAdded(void);
      virtual void handleRobotRemoved(unsigned int index, robot::ptr r);

		private:
			playtype::playtype current_playtype;

			// Create variables here (e.g. to store the roles).
			// kinda want something to 
			vector<role::ptr> roles;
			
	};

	offensive_strategy::offensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
		// Initialize variables here (e.g. create the roles).
    the_team->signal_robot_added().connect(sigc::mem_fun(*this, &offensive_strategy::handleRobotAdded));
    the_team->signal_robot_removed().connect(sigc::mem_fun(*this, &offensive_strategy::handleRobotRemoved));
		/*
		 offender and defender doesn't exist yet >"< 
		 role::offender off = role::offender(ball, field, team);
		 roles.push_back(off);     			  
		 role::defender def = role::defender(ball, field, team);
		 roles.push_back(def);
		*/
		
	}

	void offensive_strategy::update() {

		// There should be a switch statement to take care of the different playtypes
		// the implementation below is just for the "play" playtype
		// also the implementation for setting role goalie may be wrong

		// Use the variables "the_ball", "the_field", and "the_team" to allocate players to roles.
		
		// calls role::update

		// ultimate offensive strategy: all 5 players switch to offenders (including goalie, mwahahahahahaha)
		// fires once the ball gets to the other side 
		if (the_ball->position().x > 0) {
			for (int i = 0 ; i < 5 ; i++) {
				// set every player to offenders, mwahahaha 
				// the_team->get_robot(i)
			}
		}

		/*
		// else if the enemy got more than 3 robots flooding our side of the field, 2 offender, 2 defender, and 1 goalie?
		// if you want to have a stronger defence, change to defensive strategy, I believe this is the best defensive mode for offensive strategy
		else if (checkNumEnemyOnOurField()){
			// set closest offender to goalie to defender
			 

		}
		*/
		// default: 2 offenders, 1 defender, and 1 goalie?
		else {
			
			// set three closest player to ball to offender
			for (int i = 0 ; i < 3 ; i++){
				// the_team->get_robot(i).position().x;
				// the_team->get_robot(i).position().y;
				// the_ball->position().x;
				// the_ball->position().y;
			}
			// set closest player to goal to goalie

			// set closest player to goalie as defender

		}


	}

	void offensive_strategy::set_playtype(playtype::playtype t) {
		current_playtype = t;
	}
	
	Gtk::Widget *offensive_strategy::get_ui_controls() {
		return 0;
	}

  void offensive_strategy::handleRobotAdded(void){
  }

  void offensive_strategy::handleRobotRemoved(unsigned int index, robot::ptr r){
  }

	class offensive_strategy_factory : public virtual strategy_factory {
		public:
			offensive_strategy_factory();
			virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	offensive_strategy_factory::offensive_strategy_factory() : strategy_factory("Offensive Strategy") {
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

