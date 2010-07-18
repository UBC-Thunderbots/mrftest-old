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

	class DribbleStrategyFactory : public StrategyFactory {
		public:
			DribbleStrategyFactory();
			Strategy::ptr create_strategy(World::ptr world);
	};

	DribbleStrategyFactory::DribbleStrategyFactory() : StrategyFactory("Dribble Strategy") {
	}

	Strategy::ptr DribbleStrategyFactory::create_strategy(World::ptr world) {
		Strategy::ptr s(new DribbleStrategy(world));
		return s;
	}

	DribbleStrategyFactory factory;
}

DribbleStrategy::DribbleStrategy(World::ptr world) : the_world(world) {
	update_wait =0;
}

void DribbleStrategy::tick() {	
	
	const FriendlyTeam &the_team(the_world->friendly);
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

Gtk::Widget *DribbleStrategy::get_ui_controls() {
	return NULL;
}




StrategyFactory &DribbleStrategy::get_factory() {
	return factory;
}




