#include "ai/strategy/dribble_strategy.h"
#include "ai/tactic/chase.h"
#include "ai/role/defensive.h"
#include "ai/role/goalie.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_free_kick.h" 
#include "ai/role/pit_stop.h"   
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/kickoff_friendly.h"             
#include "ai/role/penalty_friendly.h"
#include "ai/role/penalty_enemy.h"
#include "ai/role/victory_dance.h"


#include <iostream>

namespace {

	class dribble_strategy_factory : public strategy_factory {
		public:
			dribble_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	dribble_strategy_factory::dribble_strategy_factory() : strategy_factory("Dribble Strategy") {
	}

	strategy::ptr dribble_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new dribble_strategy(world));
		return s;
	}

	dribble_strategy_factory factory;
}

dribble_strategy::dribble_strategy(world::ptr world) : the_world(world) {
	update_wait =0;
}

void dribble_strategy::tick() {	
	
	const friendly_team &the_team(the_world->friendly);
	update_wait = (update_wait+1) %15;
	
	for(unsigned int i=0; i<the_team.size(); i++){
	if(update_wait == 0){
		std::cout<<" bot"<< the_team.get_player(i)->pattern_index <<" set point:" << dribble_speed << " rpm: " << the_team.get_player(i)->dribbler_speed()<< std::endl;	
	}
	}
	
	if(update_wait ==0){	
	dribble_speed++;
	}
	
	for(int i=0; i<the_team.size(); i++){
		double set_point = static_cast<double>(dribble_speed)/1000.0;
		the_team.get_player(i)->move(the_team.get_player(i)->position(), the_team.get_player(i)->orientation());
		the_team.get_player(i)->dribble(set_point);	
	}
	

}

Gtk::Widget *dribble_strategy::get_ui_controls() {
	return NULL;
}




strategy_factory &dribble_strategy::get_factory() {
	return factory;
}




