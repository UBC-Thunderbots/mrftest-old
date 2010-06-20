#include "ai/util.h"
#include "ai/role/goalie.h"
#include "ai/tactic/move.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/shoot.h"
#include "geom/angle.h"
#include "geom/util.h"

#include "uicomponents/param.h"

#include <iostream>
#include <vector>

namespace {
	double_param STANDBY_DIST("Goalie distance to goal post", 0.05, 0.1, 1.0);

	const double LANE_CLEAR_PENALTY = 1.3;
	const double DIST_ADV_PENALTY = 1.1;

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

void goalie::run_goalie_confidence(player::ptr goalie, const unsigned int& flags) {

	const team& enemy(the_world->enemy);
	const friendly_team &friendly(the_world->friendly);
	const point back_of_goal(-0.5 * the_world->field().length() - 0.2, 0);	
	point ball_position = the_world->ball()->position();
	double confidence = 1.0;

	unsigned int shot_robot_idx = 0;
	bool has_shot_robot = false;
	for (size_t i = 0; i < enemy.size(); ++i) {
		if (ai_util::ball_close(the_world, enemy.get_robot(i))) {
			shot_robot_idx = i;
			has_shot_robot = true;
			break;
		}
	}

	if (has_shot_robot) {
		robot::ptr shot_robot = enemy.get_robot(shot_robot_idx);
		point shot_pos = shot_robot->position() - back_of_goal;

		for (size_t i = 0; i < enemy.size(); ++i) {
			if (i == shot_robot_idx)
				continue;

			robot::ptr robot = enemy.get_robot(i);
			point robot_pos = robot->position() - back_of_goal;

			std::vector<point> obstacles;
			for (size_t i = 0; i < enemy.size(); ++i) {
				if (i == shot_robot_idx)
					continue;

				obstacles.push_back(enemy.get_robot(i)->position());
			}

			// 0 is always goalie
			for (size_t i = 1; i < friendly.size(); ++i) {
				const player::ptr fpl = friendly.get_player(i);
				obstacles.push_back(fpl->position());
			}

			// calculates confidence score here
			bool lane_clear = ai_util::path_check(back_of_goal, robot->position(), obstacles, robot::MAX_RADIUS);
			bool dist_adv = robot->position().x < shot_robot->position().x;

			double rel_angle = 1.0 - angle_diff(robot_pos.orientation(), shot_pos.orientation()) / M_PI;

			double angle_penalty = rel_angle * rel_angle;
			confidence *= angle_penalty;

			if (lane_clear)
				confidence *= pow(angle_penalty, LANE_CLEAR_PENALTY);
			if (dist_adv)
				confidence *= pow(angle_penalty, DIST_ADV_PENALTY);
		}

	}
	//		std::cout << "confidence = " << confidence << std::endl;

	const field& f = the_world->field();
	point dir = (ball_position - back_of_goal).norm();

	point A = line_intersect(back_of_goal, back_of_goal + dir, point(-f.length() / 2, f.goal_width() / 2), point(-f.length() / 2, -f.goal_width() / 2));
	// std::cout << "point A = " << A << std::endl;
	/*
	   std::vector<point> B = line_circle_intersect(point(-f.length() / 2, 0), the_world->field().defense_area_radius(), back_of_goal, back_of_goal + dir);
	   point C;		
	   if (B.size() != 2) {
	   std::cerr << "goalie: circle line intersect error" << std::endl;
	   C = f.friendly_goal() + (ball_position - f.friendly_goal()).norm();
	   } else {
	   C = (B[0].x < B[1].x) ? B[1] : B[0];
	   }
	   */

	move move_tactic(goalie, the_world);
	move_tactic.set_position(A + dir * confidence * f.defense_area_radius());
	move_tactic.set_flags(flags);
	move_tactic.tick();
}

void goalie::run_goalie_old(const unsigned int& flags) {
	const point default_pos = point(-0.45*the_world->field().length(), 0);
	const point centre_of_goal = point(-0.5*the_world->field().length(), 0);
	const player::ptr me = the_robots[0];
	move move_tactic(me, the_world);
	point tempPoint = the_world->ball()->position()-centre_of_goal;
	tempPoint = tempPoint * (STANDBY_DIST / tempPoint.len());
	tempPoint += centre_of_goal;
	move_tactic.set_position(tempPoint);
	move_tactic.set_flags(flags);
	move_tactic.tick();
}

void goalie::run_vel_goalie(player::ptr me, const unsigned int& flags) {
	point ball_velocity = the_world->ball()->est_velocity();
	if (ball_velocity.len() > ai_util::VEL_CLOSE)
	{
		// Generic defense. // author: Koko
		const point default_pos = point( -0.5 * the_world->field().length() + the_world->field().defense_area_radius(), 0 );
		const point centre_of_goal = point( -0.5 * the_world->field().length(), 0 );
		const double defense_width = the_world->field().defense_area_stretch() + the_world->field().defense_area_radius() * 2;
		point ball_position = the_world->ball()->position();
		double reach = 0.0 ;
		point tempPoint;
		if( fabs(ball_velocity.x) > ai_util::VEL_CLOSE )
		{
			reach = ( default_pos.x - ball_position.x ) / fabs(ball_velocity.x);
			tempPoint = point( default_pos.x, ball_position.y + reach * ball_velocity.y );
		}
		else
		{
			tempPoint = default_pos;
		}
		move move_tactic(me, the_world);
		//printf( "ball velocity: %lf, %lf \n", ball_velocity.x, ball_velocity.y );
		//printf( "stay at: %lf, %lf \n", tempPoint.x, tempPoint.y);
		if( tempPoint.y < defense_width/2 && tempPoint.y > -defense_width/2 )
			// move if ball move towards the goal
		{
			move_tactic.set_position(tempPoint);
		}
		else
		{
			move_tactic.set_position(default_pos);
		}
		move_tactic.set_flags(flags);
		move_tactic.tick();

}
else
{
	point ball_position = the_world->ball()->position();
	//const double defense_width = the_world->field().defense_area_stretch() + the_world->field().defense_area_radius() * 2;
	if ( ai_util::point_in_defense( the_world, ball_position ) )
	{
		chase whoosh (me, the_world);
		whoosh.set_flags(flags);
		whoosh.tick();
	}
	else
	{
		move stay (me, the_world);
		stay.set_position(me->position());
		stay.set_flags(flags);
		stay.tick();
	}
}
}

void goalie::tick() {
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	flags &= ~(ai_flags::avoid_friendly_defence);
	flags &= ~(ai_flags::avoid_enemy_defence);

	if (the_robots.size() < 1) return;
	if (the_robots.size() > 1) {
		std::cerr << "goalie role: multiple robots!" << std::endl;
	}

	const player::ptr me = the_robots[0];

	if (the_world->playtype() == playtype::prepare_kickoff_friendly
			|| the_world->playtype() == playtype::prepare_kickoff_enemy) {
		// push the goalie away if far away from some distance
		const field& f = the_world->field();
		point wantdest = me->position();
		if (me->position().x > -f.length() / 4) {
			if (me->position().y < 0) {
				wantdest.y = - 2 * f.width() / 2;
			} else {
				wantdest.y = 2 * f.width() / 2;
			}
			if (fabs(me->position().y) > f.width() / 4) {
				wantdest.x = f.friendly_goal().x;
			}
		} else {
			wantdest = f.friendly_goal();
		}
		move tactic(me, the_world);
		tactic.set_position(wantdest);
		tactic.set_flags(flags);
		tactic.tick();
		return;
	}

	if (ai_util::posses_ball(the_world, me)) {
		// find someone to pass to
		const friendly_team& friendly = the_world->friendly;
		std::vector<player::ptr> friends;
		for (size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get_player(i) == me) continue;
			friends.push_back(me);
		}
		int bestpass = ai_util::choose_best_pass(the_world, friends);

		if (bestpass != -1) {
			// pass to someone
			pass tactic(me, the_world, friends[bestpass]);
			tactic.set_flags(flags);
			tactic.tick();
		} else {
			// shoot furthest away from the person

			// shoot to enemy goal, or randomly
			shoot tactic(me, the_world);
			tactic.set_flags(flags);
			tactic.force();
			tactic.tick();
		}

	} else {
#warning the goalie cant hold the ball for too long, it should chip somewhere very randomly
		// run_vel_goalie(me, flags);
		//run_goalie_confidence(me, flags);
		run_goalie_old(flags);
	}
}

void goalie::robots_changed() {
}

