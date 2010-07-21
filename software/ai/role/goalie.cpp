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
	DoubleParam STANDBY_DIST("Goalie distance to goal post", 0.05, 0.05, 1.0);
	BoolParam USE_OLD_GOALIE("Goalie use old code", false);

	const double LANE_CLEAR_PENALTY = 1.3;
	const double DIST_ADV_PENALTY = 1.1;

	class GoalieState : public Player::State {
		public:
			typedef RefPtr<GoalieState> Ptr;
			GoalieState(bool is_goal):is_goalie(is_goal){
			}
			bool is_goalie;
	};
}

Goalie::Goalie(World::Ptr world) : the_world(world) {
}

void Goalie::run_goalie_confidence(Player::Ptr goalie, const unsigned int& flags) {

	const Team& enemy(the_world->enemy);
	const FriendlyTeam &friendly(the_world->friendly);
	const Point back_of_goal(-0.5 * the_world->field().length() - 0.2, 0);	
	Point ball_position = the_world->ball()->position();
	double confidence = 1.0;

	unsigned int shot_robot_idx = 0;
	bool has_shot_robot = false;
	for (size_t i = 0; i < enemy.size(); ++i) {
		if (AIUtil::ball_close(the_world, enemy.get_robot(i))) {
			shot_robot_idx = i;
			has_shot_robot = true;
			break;
		}
	}

	if (has_shot_robot) {
		Robot::Ptr shot_robot = enemy.get_robot(shot_robot_idx);
		Point shot_pos = shot_robot->position() - back_of_goal;

		for (size_t i = 0; i < enemy.size(); ++i) {
			if (i == shot_robot_idx)
				continue;

			Robot::Ptr robot = enemy.get_robot(i);
			Point robot_pos = robot->position() - back_of_goal;

			std::vector<Point> obstacles;
			for (size_t i = 0; i < enemy.size(); ++i) {
				if (i == shot_robot_idx)
					continue;

				obstacles.push_back(enemy.get_robot(i)->position());
			}

			// 0 is always goalie
			for (size_t i = 1; i < friendly.size(); ++i) {
				const Player::Ptr fpl = friendly.get_player(i);
				obstacles.push_back(fpl->position());
			}

			// calculates confidence score here
			bool lane_clear = AIUtil::path_check(back_of_goal, robot->position(), obstacles, Robot::MAX_RADIUS);
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

	const Field& f = the_world->field();
	Point dir = (ball_position - back_of_goal).norm();

	Point A = line_intersect(back_of_goal, back_of_goal + dir, Point(-f.length() / 2, f.goal_width() / 2), Point(-f.length() / 2, -f.goal_width() / 2));
	// std::cout << "point A = " << A << std::endl;
	/*
	   std::vector<Point> B = line_circle_intersect(Point(-f.length() / 2, 0), the_world->field().defense_area_radius(), back_of_goal, back_of_goal + dir);
	   Point C;		
	   if (B.size() != 2) {
	   std::cerr << "goalie: circle line intersect error" << std::endl;
	   C = f.friendly_goal() + (ball_position - f.friendly_goal()).norm();
	   } else {
	   C = (B[0].x < B[1].x) ? B[1] : B[0];
	   }
	   */

	Move move_tactic(goalie, the_world);
	Point G = A + dir * confidence * f.defense_area_radius();
	G.x = std::max(G.x, - f.length() / 2 + Robot::MAX_RADIUS);
	//move_tactic.set_position(A + dir * confidence * f.defense_area_radius());
	move_tactic.set_position(G);
	move_tactic.set_flags(flags);
	move_tactic.tick();
}

void Goalie::run_goalie_old(const unsigned int& flags) {
	const Point default_pos = Point(-0.45*the_world->field().length(), 0);
	const Point centre_of_goal = Point(-0.5*the_world->field().length(), 0);
	const Player::Ptr me = players[0];
	Move move_tactic(me, the_world);
	Point tempPoint = the_world->ball()->position()-centre_of_goal;
	tempPoint = tempPoint * (STANDBY_DIST / tempPoint.len());
	tempPoint += centre_of_goal;
	move_tactic.set_position(tempPoint);
	move_tactic.set_flags(flags);
	move_tactic.tick();
}

void Goalie::run_vel_goalie(Player::Ptr me, const unsigned int& flags) {
	Point ball_velocity = the_world->ball()->est_velocity();
	if (ball_velocity.len() > AIUtil::VEL_CLOSE)
	{
		// Generic defense. // author: Koko
		const Point default_pos = Point( -0.5 * the_world->field().length() + the_world->field().defense_area_radius(), 0 );
		const Point centre_of_goal = Point( -0.5 * the_world->field().length(), 0 );
		const double defense_width = the_world->field().defense_area_stretch() + the_world->field().defense_area_radius() * 2;
		Point ball_position = the_world->ball()->position();
		double reach = 0.0 ;
		Point tempPoint;
		if( fabs(ball_velocity.x) > AIUtil::VEL_CLOSE )
		{
			reach = ( default_pos.x - ball_position.x ) / fabs(ball_velocity.x);
			tempPoint = Point( default_pos.x, ball_position.y + reach * ball_velocity.y );
		}
		else
		{
			tempPoint = default_pos;
		}
		Move move_tactic(me, the_world);
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
	Point ball_position = the_world->ball()->position();
	//const double defense_width = the_world->field().defense_area_stretch() + the_world->field().defense_area_radius() * 2;
	if ( AIUtil::point_in_defense( the_world, ball_position ) )
	{
		Chase whoosh (me, the_world);
		whoosh.set_flags(flags);
		whoosh.tick();
	}
	else
	{
		Move stay (me, the_world);
		stay.set_position(me->position());
		stay.set_flags(flags);
		stay.tick();
	}
}
}

void Goalie::tick() {
	unsigned int flags = AIFlags::calc_flags(the_world->playtype());
	flags &= ~(AIFlags::AVOID_FRIENDLY_DEFENSE);
	flags &= ~(AIFlags::AVOID_ENEMY_DEFENSE);
	flags &= ~(AIFlags::AVOID_BALL_STOP);

	if (players.size() < 1) return;
	if (players.size() > 1) {
		std::cerr << "goalie role: multiple robots!" << std::endl;
	}

	const Player::Ptr me = players[0];

	if (the_world->playtype() == PlayType::PREPARE_KICKOFF_FRIENDLY
			|| the_world->playtype() == PlayType::PREPARE_KICKOFF_ENEMY) {
		// push the goalie away if far away from some distance
		const Field& f = the_world->field();
		Point wantdest = me->position();
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
		Move tactic(me, the_world);
		tactic.set_position(wantdest);
		tactic.set_flags(flags);
		tactic.tick();
		return;
	}

	if (AIUtil::posses_ball(the_world, me)) {
		// find someone to pass to
		const FriendlyTeam& friendly = the_world->friendly;
		std::vector<Player::Ptr> friends;
		for (size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get_player(i) == me) continue;
			friends.push_back(me);
		}
		int bestpass = AIUtil::choose_best_pass(the_world, friends);

		if (bestpass != -1) {
			// pass to someone
			Pass tactic(me, the_world, friends[bestpass]);
			tactic.set_flags(flags);
			tactic.tick();
		} else {
			// shoot furthest away from the person

			// shoot to enemy goal, or randomly
			Shoot tactic(me, the_world);
			tactic.set_flags(flags);
			tactic.force();
			tactic.tick();
		}

	} else {
#warning the goalie cant hold the ball for too long, it should chip somewhere very randomly
		// run_vel_goalie(me, flags);
		if (USE_OLD_GOALIE) run_goalie_old(flags);
		else run_goalie_confidence(me, flags);
	}
}

void Goalie::players_changed() {
}

