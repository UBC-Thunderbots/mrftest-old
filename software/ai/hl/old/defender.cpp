#include "ai/hl/util.h"
#include "ai/hl/old/defender.h"
#include "ai/hl/old/tactics.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/param.h"
#include <iostream>
#include <utility>

using AI::HL::Defender;
using namespace AI::HL::W;

namespace {
	DoubleParam lone_goalie_dist("Lone goalie distance to goal post (m)", "oldAI", 0.20, 0.05, 1.0);

	DoubleParam max_goalie_dist("max goalie dist from goal (robot radius)", "oldAI", 3.0, 0.0, 10.0);

	DoubleParam robot_shrink("shrink robot radius", "oldAI", 1.1, 0.1, 2.0);

	// DoubleParam goalie_chase_thresh("max distance from goal for goalie to chase ball (field width)", 0.25, 0.0, 0.5);

	BoolParam Goalie_dives("goalie dive for the ball", "oldAI", true);

	DoubleParam negligible_velocity("goalie should ignore direction of ball", "oldAI", 0.05, 1e-4, 1.0);

	std::pair<Point, bool> get_ramball_location(Point dst, AI::HL::W::World &world, AI::HL::W::Player::Ptr player) {
		Point ball_dir = world.ball().velocity();

		if (ball_dir.lensq() < negligible_velocity) {
			return std::make_pair(world.ball().position(), false);
		}

		if (unique_line_intersect(player->position(), dst, world.ball().position(), world.ball().position() + ball_dir)) {
			// std::cout<<" has intersection "<<std::endl;

			Point location = line_intersect(player->position(), dst, world.ball().position(), world.ball().position() + ball_dir);
			// timespec intersect = world.monotonic_time();
			// timespec_add(intersect, double_to_timespec((location -  world.ball().position()).len()/world.ball().velocity().len()), intersect);

			Point vec1 = location - player->position();
			Point vec2 = dst - player->position();

			Point ball_vec = location - world.ball().position();

			if (vec1.dot(vec2) > 0 && ball_dir.dot(ball_vec)) {
				return std::make_pair(location, true);
			}
		}

		// if everything fails then just stay put
		return std::make_pair(player->position(), false);
	}
}

// double Defender::get_goalie_chase_thresh() {
// return goalie_chase_thresh;
// }

Defender::Defender(World &w) : world(w), chase(false) {
}

void Defender::set_players(const std::vector<Player::Ptr> &p, Player::Ptr g) {
	if (!g.is()) {
		// LOG_ERROR("no goalie");
		if (p.size() > 0) {
			LOG_ERROR("cannot have defenders w/o goalie");
		}
	}
	// if (p.size() == 0) {
	// LOG_ERROR("no defender to accompany goalie");
	// }
	players = p;
	goalie = g;
}

std::pair<Point, std::vector<Point> > Defender::calc_block_positions() const {
	const Field &f = world.field();

	std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

	// sort enemies by distance to own goal
	std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(f.friendly_goal()));

	// list of points to defend, by order of importance
	std::vector<Point> waypoints;

	// there is cone ball to goal sides, bounded by 1 rays.
	// the side that goalie is going to guard is goal_side
	// the opposite side is goal_opp
	// a defender will guard goal_opp
	const Point ball_pos = world.ball().position();
	const Point goal_side = goalie_top ? Point(-f.length() / 2, f.goal_width() / 2) : Point(-f.length() / 2, -f.goal_width() / 2);
	const Point goal_opp = goalie_top ? Point(-f.length() / 2, -f.goal_width() / 2) : Point(-f.length() / 2, f.goal_width() / 2);

	// now calculate where you want the goalie to be
	Point goalie_pos;

	const double radius = Robot::MAX_RADIUS * robot_shrink;

	{
		// distance on the goalside - ball line that the robot touches
		const Point ball2side = goal_side - ball_pos;
		const Point touch_vec = -ball2side.norm(); // side to ball
		const double touch_dist = std::min(-ball2side.x / 2, max_goalie_dist * Robot::MAX_RADIUS);
		const Point perp = (goalie_top) ? touch_vec.rotate(-M_PI / 2) : touch_vec.rotate(M_PI / 2);
		goalie_pos = goal_side + touch_vec * touch_dist + perp * radius;

		// prevent the goalie from entering the goal area
		goalie_pos.x = std::max(goalie_pos.x, -f.length() / 2 + radius);


		// check if goalie needs to "dive to save ball
		std::pair<Point, bool> temp = get_ramball_location(goal_side, world, goalie);

		if (!temp.second) {
			temp = get_ramball_location(goal_opp, world, goalie);
		}
		bool goalie_dive = temp.second;

		if (goalie_dive && Goalie_dives) {
			// std::cout<<" goal dive "<<std::endl;

			if (temp.first.y >= -f.goal_width() && temp.first.y <= f.goal_width()) {
				goalie_pos = temp.first;
				// prevent the goalie from entering the goal area
				goalie_pos.x = std::max(goalie_pos.x, -f.length() / 2 + radius);
			}
		}
	}

	// first defender will block the remaining cone from the ball
	{
		Point D1 = calc_block_cone_defender(goal_side, goal_opp, ball_pos, goalie_pos, radius);
		bool blowup = false;
		if (D1.x < Robot::MAX_RADIUS - f.length() / 2 + f.defense_area_stretch()) {
			blowup = true;
		}
		if (std::fabs(D1.y) > f.width() / 4) {
			blowup = true;
		}
		if (blowup) {
			D1 = (f.friendly_goal() + ball_pos) / 2;
		}
		waypoints.push_back(D1);
	}

	// next two defenders block nearest enemy sights to goal if needed
	// enemies with ball possession are ignored (they should be handled above)
	for (size_t i = 0; i < enemies.size() && waypoints.size() < players.size(); ++i) {
		if (!AI::HL::Util::ball_close(world, enemies[i])) {
			bool blowup = false;
			Point D = calc_block_cone(goal_side, goal_opp, enemies[i]->position(), radius);
			if (D.x < Robot::MAX_RADIUS - f.length() / 2 + f.defense_area_stretch()) {
				blowup = true;
			}
			if (std::fabs(D.y) > f.width() / 4) {
				blowup = true;
			}
			if (blowup) {
				D = (f.friendly_goal() + enemies[i]->position()) / 2;
			}
			waypoints.push_back(D);
		}
	}

	// there are too few enemies, this is strange
	while (waypoints.size() < players.size()) {
		waypoints.push_back((f.friendly_goal() + ball_pos) / 2);
	}

	return std::make_pair(goalie_pos, waypoints);
}

void Defender::tick() {
	// if (players.size() == 0) {
	// if (goalie.is()) {
	// AI::HL::Tactics::lone_goalie(world, goalie);
	// }
	// LOG_WARN("no robots");
	// }

	if (!goalie.is()) {
		LOG_ERROR("no goalie");
		return;
	}

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));

	const Point ball_pos = world.ball().position();

	// choose whether to defend top or bottom
	if (ball_pos.y > Robot::MAX_RADIUS * 2) {
		goalie_top = true;
	} else if (ball_pos.y < -Robot::MAX_RADIUS * 2) {
		goalie_top = false;
	}

	std::pair<Point, std::vector<Point> > positions = calc_block_positions();
	std::vector<Point> &waypoints = positions.second;

	// just ignore unnecessary waypoints for now
	while (waypoints.size() > players.size()) {
		waypoints.pop_back();
	}

	// do matching for distances
	std::vector<Point> locations;
	for (std::size_t i = 0; i < players.size(); ++i) {
		locations.push_back(players[i]->position());
	}
	std::vector<std::size_t> order = dist_matching(locations, waypoints);

	// other players not part of this role.
	std::vector<Player::Ptr> supporters;
	FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 1; i < friendly.size(); ++i) {
		if (!exists(players.begin(), players.end(), friendly.get(i))) {
			supporters.push_back(friendly.get(i));
		}
	}

	// figure out who should chase the ball
	Player::Ptr chaser;
	if (AI::HL::Util::point_in_friendly_defense(world.field(), ball_pos)) {
		// emergency, chase the ball
		chaser = goalie;
	} else if (chase) {
		double best_dist = 1e99;

		// goalie should chase the ball
		// if and only if the ball is some near distance from the goal area
		/*
		   if (players.size() == 0 || (world.ball().position() - world.field().friendly_goal()).len() < world.field().length() * goalie_chase_thresh) {
		   chaser = goalie;
		   best_dist = (goalie->position() - world.ball().position()).len();
		   }
		 */

		for (std::size_t i = 0; i < players.size(); ++i) {
			double dist = (players[i]->position() - world.ball().position()).len();
			if (dist < best_dist) {
				chaser = players[i];
				best_dist = dist;
			}
		}
	}

	// TODO: print out the chaser

	// TODO: goalie rush defence

	// TODO: if in possesion of ball, do something!

	// now do movement
	unsigned int defender_flags = AI::Flags::calc_flags(world.playtype());
	unsigned int goalie_flags = defender_flags & ~AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE;

	// all defenders
	{
		std::size_t w = 0; // so we can skip robots as needed
		for (std::size_t i = 0; i < players.size(); ++i) {
			if (w >= waypoints.size()) {
				// LOG_WARN(Glib::ustring::compose("%1 nothing to do", players[i]->name));
				// move::ptr tactic(new move(players[i], the_world));
				// tactic->set_position(players[i]->position());
				// assign(players[i], tactic);
				continue;
			}

			if (chaser == players[i]) {
				Player::Ptr passee = AI::HL::Util::choose_best_pass(world, supporters);

				// if defender can shoot, go ahead, else try to pass or repel
				if (AI::HL::Util::calc_best_shot(world, players[i], Robot::MAX_RADIUS).second > AI::HL::Util::shoot_accuracy) {
					AI::HL::Tactics::shoot(world, players[i], defender_flags, 10.0);
				} else if (passee.is()) {
					AI::HL::Tactics::pass(world, players[i], passee, defender_flags);
				} else {
					AI::HL::Tactics::repel(world, players[i], defender_flags);
				}
			} else {
				// move to defense position
				players[i]->move(waypoints[order[w]], (world.ball().position() - players[i]->position()).orientation(), defender_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			}
			++w;
		}
	}

	// goalie
	if (chaser == goalie) {
		AI::HL::Tactics::repel(world, goalie, goalie_flags);
	} else {
		goalie->move(positions.first, (world.ball().position() - goalie->position()).orientation(), goalie_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}
}

