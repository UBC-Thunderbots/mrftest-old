#include "ai/util.h"
#include "ai/role/goalie.h"
#include "ai/tactic/move.h"
#include "ai/tactic/pass.h"

#include <iostream>

namespace {
	const double STANDBY_DIST = 0.2;

	class goalie_state : public player::state {
		public:
			typedef Glib::RefPtr<goalie_state> ptr;
	  goalie_state(bool is_goal):is_goalie(is_goal){
			}
	  bool is_goalie;
	};
}

goalie::goalie(world::ptr world) : the_world(world) {
}

void goalie::tick() {
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	flags &= ~(ai_flags::avoid_friendly_defence);
	
	if (the_robots.size() < 1) return;
	if (the_robots.size() > 1) {
		std::cerr << "goalie role: multiple robots!" << std::endl;
	}

	const player::ptr me = the_robots[0];

	if (ai_util::posses_ball(the_world, me)) {
		// TODO: check correctness
		// Code copied from defensive role

		std::vector<player::ptr> friends = ai_util::get_friends(the_world->friendly, the_robots);
		std::sort(friends.begin(), friends.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));

		int nearidx = -1;
		for (size_t i = 0; i < friends.size(); ++i) {
			if (!ai_util::can_receive(the_world, friends[i])) continue;
			nearidx = i;
			break;
		}

		if (nearidx == -1) {
			move move_tactic(me, the_world);
			move_tactic.set_position(me->position());
			move_tactic.set_flags(flags);
			move_tactic.tick();
		} else {
			pass pass_tactic(me, the_world, friends[nearidx]);
			pass_tactic.set_flags(flags);
			pass_tactic.tick();
		}

#warning the goalie cant hold the ball for too long, it should chip somewhere very randomly

	} else {
	
		// Generic defence. // author: Koko
		const point default_pos = point( -0.5 * the_world->field().length() + the_world->field().defense_area_radius(), 0);
		const point centre_of_goal = point( -0.5*the_world->field().length(), 0);
		const int field_width = the_world->field().width();
		point ball_position = the_world->ball()->position();
		point ball_velocity = the_world->ball()->est_velocity();
		double reach;
		if( ball_velocity.x != 0.0 )
			reach = ( centre_of_goal.x - ball_velocity.x ) / ball_velocity.x;
		point tempPoint = point( centre_of_goal.x, ball_position.y + reach * ball_velocity.y );
		move move_tactic(me, the_world);
		printf( "ball velocity: %lf, %lf \n", ball_velocity.x, ball_velocity.y );
		printf( "stay at: %lf, %lf \n", tempPoint.x, tempPoint.y);
		//if( tempPoint.y < field_width * 1 / 4 && tempPoint.y > -field_width * 1 / 4 )
		// move if ball move towards the goal
		//{
			move_tactic.set_position(tempPoint);
		//}
		//else
		//{
		//	move_tactic.set_position(me->position());
		//}
		move_tactic.set_flags(flags);
		move_tactic.tick();
		
		
		/*// Generic defence. older version
		const point default_pos = point(-0.45*the_world->field().length(), 0);
		const point centre_of_goal = point(-0.5*the_world->field().length(), 0);
		move move_tactic(me, the_world);
		point tempPoint = the_world->ball()->position()-centre_of_goal;
		tempPoint = tempPoint * (STANDBY_DIST / tempPoint.len());
		tempPoint += centre_of_goal;
		move_tactic.set_position(tempPoint);
		move_tactic.set_flags(flags);
		move_tactic.tick();*/
	} 
}

void goalie::robots_changed() {
}

