#include "ai/navigator/util.h"
#include "ai/flags.h"
#include <vector>

using namespace AI::Flags;
using namespace AI::Nav::W;

bool AI::Nav::Util::valid_dst(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	return true;
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {

	if(!valid_dst(cur, world, player)){
		return valid_dst(dst, world, player);
	}
	
	unsigned int flags = player->flags();
	double player_rad = player->MAX_RADIUS;

	// avoid enemy robots
	for (unsigned int i = 0; i < world.enemy_team().size(); i++) {
		AI::Nav::W::Robot::Ptr rob = world.enemy_team().get(i);
		double enemy_rad = rob->MAX_RADIUS;
		if (line_circle_intersect(rob->position(), player_rad + enemy_rad, cur, dst).size() > 0) {
			return false;
		}
	}
	// avoid friendly robots
	for (unsigned int i = 0; i < world.friendly_team().size(); i++) {
		AI::Nav::W::Player::Ptr rob = world.friendly_team().get(i);
		double friendly_rad = rob->MAX_RADIUS;
		if (rob != player && line_circle_intersect(rob->position(), player_rad + friendly_rad, cur, dst).size() > 0) {
			return false;
		}
	}
	
	if(AI::Flags::FLAG_CLIP_PLAY_AREA & flags){

	}
	if(AI::Flags::FLAG_AVOID_BALL_STOP & flags){

	}
	if(AI::Flags::FLAG_AVOID_BALL_TINY & flags){

	}
	if(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE & flags){

	}
	if(AI::Flags::FLAG_AVOID_ENEMY_DEFENSE & flags){

	}
	if(AI::Flags::FLAG_STAY_OWN_HALF & flags){

	}
	if(AI::Flags::FLAG_PENALTY_KICK_FRIENDLY & flags){

	}
	if(AI::Flags::FLAG_PENALTY_KICK_ENEMY & flags){

	}
	return true;
}

bool AI::Nav::Util::check_dest_valid(Point dest, World &world, Player::Ptr player) {
	double AVOID_TINY_CONSTANT = 0.15; // constant for AVOID_BALL_TINY
	double AVOID_PENALTY_CONSTANT = 0.40; // constant for AVOID_PENALTY_*
	const Field &field = world.field();
	double length, width;
	length = field.length();
	width = field.width();
	unsigned int flags = player->flags();
	// check for CLIP_PLAY_AREA simple rectangular bounds
	if ((flags & FLAG_CLIP_PLAY_AREA) == FLAG_CLIP_PLAY_AREA) {
		if (dest.x < -length / 2
		    || dest.x > length / 2
		    || dest.y < -width / 2
		    || dest.y > width / 2) {
			return false;
		}
	}
	// check for STAY_OWN_HALF
	if ((flags & FLAG_STAY_OWN_HALF) == FLAG_STAY_OWN_HALF) {
		if (dest.x > 0) {
			return false;
		}
	}
	/* field information for AVOID_*_DEFENSE_AREA checks
	 * defined by defense_area_radius distance from two goal posts
	 * and rectangular stretch directly in front of goal
	 */
	Point f_post1, f_post2, e_post1, e_post2;
	double defense_area_stretch, defense_area_radius;
	defense_area_stretch = field.defense_area_stretch();
	defense_area_radius = field.defense_area_radius();
	f_post1 = Point(-length / 2, defense_area_stretch / 2);
	f_post2 = Point(-length / 2, -defense_area_stretch / 2);
	// friendly case
	if ((flags & FLAG_AVOID_FRIENDLY_DEFENSE) == FLAG_AVOID_FRIENDLY_DEFENSE) {
		if (dest.x < -(length / 2) + defense_area_radius
		    && dest.x > -length / 2
		    && dest.y < defense_area_stretch / 2
		    && dest.y > -defense_area_stretch / 2) {
			return false;
		} else if ((dest - f_post1).len() < defense_area_radius
		           || (dest - f_post2).len() < defense_area_radius) {
			return false;
		}
	}
	// enemy goal posts
	e_post1 = Point(length / 2, defense_area_stretch / 2);
	e_post2 = Point(length / 2, -defense_area_stretch / 2);
	// enemy case
	if ((flags & FLAG_AVOID_ENEMY_DEFENSE) == FLAG_AVOID_ENEMY_DEFENSE) {
		if (dest.x > length / 2 - defense_area_radius - 0.20
		    && dest.x < length / 2
		    && dest.y < defense_area_stretch / 2
		    && dest.y > -defense_area_stretch / 2) {
			return false;
		} else if ((dest - e_post1).len() < defense_area_radius + 0.20 // 20cm from defense line
		           || (dest - e_post2).len() < defense_area_radius + 0.20) {
			return false;
		}
	}
	// ball checks (lol)
	const Ball &ball = world.ball();
	// AVOID_BALL_STOP
	if ((flags & FLAG_AVOID_BALL_STOP) == FLAG_AVOID_BALL_STOP) {
		if ((dest - ball.position()).len() < 0.5) {
			return false; // avoid ball by 50cm
		}
	}
	// AVOID_BALL_TINY
	if ((flags & FLAG_AVOID_BALL_TINY) == FLAG_AVOID_BALL_TINY) {
		if ((dest - ball.position()).len() < AVOID_TINY_CONSTANT) {
			return false; // avoid ball by this constant?
		}
	}
	/* PENALTY_KICK_* checks
	 * penalty marks are defined 450mm & equidistant from both goal posts,
	 * i assume that defense_area_stretch is a close enough approximation
	 */
	Point f_penalty_mark, e_penalty_mark;
	f_penalty_mark = Point((-length / 2) + defense_area_stretch, 0);
	e_penalty_mark = Point((length / 2) - defense_area_stretch, 0);
	if ((flags & FLAG_PENALTY_KICK_FRIENDLY) == FLAG_PENALTY_KICK_FRIENDLY) {
		if ((dest - f_penalty_mark).len() < AVOID_PENALTY_CONSTANT) {
			return false;
		}
	}
	if ((flags & FLAG_PENALTY_KICK_ENEMY) == FLAG_PENALTY_KICK_ENEMY) {
		if ((dest - e_penalty_mark).len() < AVOID_PENALTY_CONSTANT) {
			return false;
		}
	}
	/* check if dest is on another robot
	 * 2*robot radius (maybe we need some sort of margin?) from center to center
	 */
	FriendlyTeam &f_team = world.friendly_team();
	EnemyTeam &e_team = world.enemy_team();
	Player::Ptr f_player;
	Robot::Ptr e_player;
	// friendly case
	for (unsigned int i = 0; i < f_team.size(); i++) {
		f_player = f_team.get(i);
		if ((f_player != player) && ((dest - f_player->position()).len() < 2 * f_player->MAX_RADIUS)) {
			return false;
		}
	}
	// enemy case
	for (unsigned int i = 0; i < e_team.size(); i++) {
		e_player = e_team.get(i);
		if ((dest - e_player->position()).len() < 2 * e_player->MAX_RADIUS) {
			return false;
		}
	}

	return true;
}

