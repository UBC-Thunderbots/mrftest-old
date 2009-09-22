#include "ai/strategy.h"

//call update() to get new play types to see which strategy method to use

//constructs new strategy
strategy::strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : the_ball(ball), the_field(field), the_team(team) {

	//God, you should have this thing get play types as a parameter 
	
}

//create different strategy methods for different play types
/*
void strategy::OffensiveStrategy(controlled_team::ptr team){
	//determines the roles of the players of te team 
	//for(int i = 0; i < 5; i++)
	//	team.player(i)->role::offender();
	//there should be some methods that determines the role of the players

}

void strategy::DefensiveStrategy(controlled_team::ptr team){
	//determines the roles of the players of te team 
	//for(int i = 0; i < 5; i++)
	//	team.player(i)->role::defender();
	//same here

}

//You can change the name of this method to SuckStrategy
void strategy::ByronStrategy(controlled_team::ptr team){
	//determines the roles of the players of te team 
	//for(int i = 0; i < 5; i++)
	//	team.player(i)->role::Byrons();
	//same here
	
}
*/
