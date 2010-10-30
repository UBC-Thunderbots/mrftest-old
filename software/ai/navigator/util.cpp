#include "ai/navigator/util.h"
#include <vector>

bool AI::Nav::Util::valid_dst(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player){
	return true;
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player){
	
	unsigned int flags = player->flags();
	double player_rad = player->MAX_RADIUS;
	
	//avoid enemy robots 
	for(unsigned int i=0; i<world.enemy_team().size(); i++){
		AI::Nav::W::Robot::Ptr rob = world.enemy_team().get(i);
		double enemy_rad = rob->MAX_RADIUS;		
		if(line_circle_intersect(rob->position(), player_rad + enemy_rad, cur, dst).size()>0)
			return false;		
	}
	//avoid friendly robots
	for(unsigned int i=0; i<world.friendly_team().size(); i++){
		AI::Nav::W::Player::Ptr rob = world.friendly_team().get(i);
		double friendly_rad = rob->MAX_RADIUS;		
		if(rob!=player && line_circle_intersect(rob->position(), player_rad + friendly_rad, cur, dst).size()>0)
			return false;		
	}
	
	return true;
}
		
