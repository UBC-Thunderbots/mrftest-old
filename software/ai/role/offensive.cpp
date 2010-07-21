#include "ai/role/offensive.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/receive.h"
#include "ai/tactic/block.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

#include <iostream>

#include "uicomponents/param.h"

namespace {

	BoolParam OFFENSIVE_PIVOT("offensive: use pivot", true);
	BoolParam OFFENSIVE_BLOCK("offensive: use block when enemy has the ball", false);

	const double SHOOT_ALLOWANCE = Ball::RADIUS;

	const double ENEMY_FACTOR = 1.0;
	const double CAN_SEE_BALL = 1.0;

	// chop up half the field into 100x100 grid
	// evaluate some functions

	const double ONE = 1.0 / 180.0 * M_PI;

	const double NEAR = Robot::MAX_RADIUS * 3;

};

Offensive::Offensive(World::ptr world) : the_world(world) {
}

double Offensive::scoring_function(const std::vector<Point>& enemypos, const Point& pos, const std::vector<Point>& dontblock) const {
	// Hmm.. not sure if having negative number is a good idea.
	std::pair<Point, double> bestshot = AIUtil::calc_best_shot(the_world->field(), enemypos, pos);
	double score = bestshot.second;

	for (size_t i = 0; i < enemypos.size(); ++i) {
		if ((enemypos[i] - pos).len() < NEAR) {
			return -1e99;
		}
	}

	// TODO: check the line below here
	// scoring factors:
	// density of enemy, passing distance, distance to the goal, angle of shooting, angle of receiving
	// distance toward the closest enemy, travel distance, behind of in front of the enemy
	// UI for viewing
	if (!AIUtil::path_check(the_world->ball()->position(), pos, enemypos, Robot::MAX_RADIUS + Ball::RADIUS + SHOOT_ALLOWANCE)) {
		return -1e99;
	}

	for (size_t i = 0; i < dontblock.size(); ++i) {
		const Point diff2 = (pos - dontblock[i]);
		if (diff2.len() < NEAR) {
			return -1e99;
		}
	}

	// super expensive calculation
	for (size_t i = 0; i < dontblock.size(); ++i) {
		std::pair<Point, double> shootershot = AIUtil::calc_best_shot(the_world->field(), enemypos, dontblock[i]);
		const Point diff1 = (shootershot.first - dontblock[i]);
		const Point diff2 = (pos - dontblock[i]);
		const double anglediff = angle_diff(diff1.orientation(), diff2.orientation());
		if (anglediff * 2 < shootershot.second) {
			return -1e99;
		}
	}

	// 10 degrees of shooting is 10 points
	score *= 10.0 / (10.0 * ONE);
	// want to be as near to our own goal as possible
	// score -= 1.0 * pos.x;
	const double balldist = (pos - the_world->ball()->position()).len();
	const double goaldist = (pos - bestshot.first).len();

	// divide by largest distance?
	const double bigdist = std::max(balldist, goaldist);
	score /= bigdist;

	return score;
}

Point Offensive::calc_position_best(const std::vector<Point>& enemypos, const std::vector<Point>& dontblock) {
	const double x1 = 0;
	const double x2 = the_world->field().length() / 2;
	const double y1 = -the_world->field().width() / 2;
	const double y2 = the_world->field().width() / 2;

	const double dx = (x2 - x1) / (GRIDX+1);
	const double dy = (y2 - y1) / (GRIDY+1);
	double bestscore = -1e50;
	Point bestpos(0, 0);
	for (int i = 0; i < GRIDX; ++i) {
		for (int j = 0; j < GRIDY; ++j) {
			if (!okaygrid[i][j]) continue;
			const double x = x1 + dx * (i + 1);
			const double y = y1 + dy * (j + 1);
			const Point pos = Point(x, y);
			// TEMPORARY HACK!!
			const double goaldist = (pos - the_world->field().enemy_goal()).len();
			if (goaldist < the_world->field().goal_width()) {
				okaygrid[i][j] = false;
				continue;
			}
			const double score = scoring_function(enemypos, pos, dontblock);
			if (score < -1e50) {
				okaygrid[i][j] = false;
				continue;
			}
			if (score > bestscore) {
				bestscore = score;
				bestpos = pos;
			}
		}
	}
	return bestpos;
}

std::vector<Point> Offensive::calc_position_best(const unsigned int n) {
	const EnemyTeam& enemy = the_world->enemy;
	std::vector<Point> enemypos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemypos.push_back(enemy.get_robot(i)->position());
	}
	// TODO: optimize using the matrix below
	for (int i = 0; i < GRIDX; ++i) {
		for (int j = 0; j < GRIDY; ++j) {
			okaygrid[i][j] = true;
		}
	}
	std::vector<Point> dontblock;
	dontblock.push_back(the_world->ball()->position());
	std::vector<Point> ret;
	for (size_t i = 0; i < n; ++i) {
		const Point best = calc_position_best(enemypos, dontblock);
		ret.push_back(best);
		dontblock.push_back(best);
	}
	return ret;
}

// TODO: refactor
double Offensive::get_distance_from_goal(int index) const {
	Point pos = players[index]->position();
	Point goal = Point(the_world->field().length()/2,0);
	Point dist = goal-pos;
	double distance = dist.len();
	return distance;
}

void Offensive::tick() {
	tactics.clear();
	tactics.resize(players.size());

	if (players.size() == 0) return;

	// Sort by distance to ball. DO NOT SORT AGAIN!!
	std::sort(players.begin(), players.end(), AIUtil::CmpDist<Player::ptr>(the_world->ball()->position()));

	const FriendlyTeam& friendly(the_world->friendly);
	// const Field& the_field = the_world->field();

	bool teampossesball = false;
	int baller = -1;
	for (size_t i = 0; i < players.size(); ++i) {
		if (AIUtil::posses_ball(the_world, players[i])) {
			baller = i;
			teampossesball = true;
			break;
		}
	}

	std::vector<Player::ptr> friends = AIUtil::get_friends(friendly, players);

	if (!teampossesball) {
		for (size_t i = 0; i < friends.size(); ++i) {
			if (AIUtil::posses_ball(the_world, friends[i])) {
				teampossesball = true;
				break;
			}
		}
	}

	if (baller != -1 && baller != 0) {
		LOG_WARN(Glib::ustring::compose("%1 and %2 are near the ball, but only %2 has it", players[0]->name, players[1]->name));
	}

	if (teampossesball) {
		// someone has the ball
		if (baller != -1) {
			// calculate some good positions for robots not holding the ball
			std::vector<Point> waypoints = calc_position_best(static_cast<int>(players.size()) - 1);

			// other robots not having the ball
			std::vector<Player::ptr> available;
			std::vector<Point> locations;
			for (size_t i = 0; i < players.size(); ++i) {
				if (static_cast<int>(i) == baller) continue;
				available.push_back(players[i]);
				locations.push_back(players[i]->position());
			}

			std::vector<size_t> order = dist_matching(locations, waypoints);

			size_t w = 0;
			for (size_t i = 0; i < players.size(); ++i) {
				if (static_cast<int>(i) == baller) continue;
				if (w >= waypoints.size()) {
					LOG_WARN(Glib::ustring::compose("%1 nothing to do", players[i]->name));
					Move::ptr move_tactic(new Move(players[i], the_world));
					move_tactic->set_position(players[i]->position());
					tactics[i] = move_tactic;
				} else {
					Move::ptr move_tactic(new Move(players[i], the_world));
					move_tactic->set_position(waypoints[order[w]]);
					tactics[i] = move_tactic;
				}
				++w;
			}

			int shooter = -1;
			double shooterangle = 0;

			// We will try passing to another offensive robot,
			// if there is a clear path to the passee and the passee has a clear path to the goal
			for (size_t j = 0; j < players.size(); ++j) {
				if (static_cast<int>(j) != baller && !AIUtil::can_receive(the_world, players[j])) continue;
				// if (AIUtil::calc_best_shot(robots[j], the_world) == -1) continue;
				// if (get_distance_from_goal(j) > the_world->field().length() / 2) continue;

				// TODO: create another weighting function
				double angle = AIUtil::calc_goal_visibility_angle(the_world, players[j], false);
				LOG_DEBUG(Glib::ustring::compose("%1 can see %2 degrees", players[j]->name, angle * 180.0 / M_PI));
				// the baller has more importance
				if (static_cast<int>(j) == baller) angle *= 10.0;
				if (angle > shooterangle) {
					shooter = j;
					shooterangle = angle;
				}
			}

			//if (overlay) overlay->move_to(robots[baller]->position().x, robots[baller]->position().y);

			if (shooter == baller) {
				// i shall shoot
				Shoot::ptr shoot_tactic(new Shoot(players[baller], the_world));
				if (OFFENSIVE_PIVOT) shoot_tactic->set_pivot(false);
				tactics[baller] = shoot_tactic;
				//if (overlay) overlay->line_to(the_field.enemy_goal().x, the_field.enemy_goal().y);
			} else if (shooter != -1) {
				LOG_INFO(Glib::ustring::compose("%1 pass to %2", players[baller]->name, players[shooter]->name));
				// found suitable passee, make a pass
				tactics[baller] = Pass::ptr(new Pass(players[baller], the_world, players[shooter]));
			} else if (get_distance_from_goal(baller) < the_world->field().length() / 6) {
				// very close to goal, so try making a shot anyways
				Shoot::ptr shoot_tactic(new Shoot(players[baller], the_world));
				shoot_tactic->force();
				if (OFFENSIVE_PIVOT) shoot_tactic->set_pivot(false);
				tactics[baller] = shoot_tactic;
			} else {
				// i shall shoot
				Shoot::ptr shoot_tactic(new Shoot(players[baller], the_world));
				shoot_tactic->force();
				if (OFFENSIVE_PIVOT) shoot_tactic->set_pivot(false);
				tactics[baller] = Shoot::ptr(new Shoot(players[baller], the_world));
			}
		} else {
			LOG_INFO("receive ball");
			// no one in this role has the ball
			// prepare to receive some ball
			for (size_t i = 0; i < players.size(); ++i) {
				tactics[i] = Receive::ptr(new Receive(players[i], the_world));
			}
		}
	} else {
		// calculate some good positions for robots not holding the ball
		std::vector<Point> waypoints;
		std::vector<Robot::ptr> block_targets;
		if (OFFENSIVE_BLOCK){
			const EnemyTeam& enemy = the_world->enemy;
			const Point ballpos = the_world->ball()->position();
			for (size_t i = 0; i < enemy.size(); ++i) {
				Point enemy_pos = enemy.get_robot(i)->position();
				if ((enemy_pos - ballpos).len() > 0.5 + Robot::MAX_RADIUS)
					block_targets.push_back(enemy.get_robot(i));
			}
			std::sort(block_targets.begin(),block_targets.end(), AIUtil::CmpDist<Robot::ptr>(the_world->field().friendly_goal()));
			for (size_t i = 0; i < block_targets.size() && waypoints.size() + 1 < players.size(); i++)
				waypoints.push_back(block_targets[i]->position());
			if (waypoints.size() + 1 < players.size()){
				std::vector<Point> temp = calc_position_best(static_cast<int>(players.size()) - 1 - waypoints.size());
				for (size_t i = 0; i < temp.size(); i++)
					waypoints.push_back(temp[i]);
			}
		}
		else waypoints = calc_position_best(static_cast<int>(players.size()) - 1);

		// other robots not having the ball
		std::vector<Player::ptr> available;
		std::vector<Point> locations;
		for (size_t i = 1; i < players.size(); ++i) {
			available.push_back(players[i]);
			locations.push_back(players[i]->position());
		}

		std::vector<size_t> order = dist_matching(locations, waypoints);
		if (OFFENSIVE_BLOCK){
			size_t w = 0;
			for (size_t i = 1; i < players.size(); ++i) {
				if (w >= waypoints.size()) {
					LOG_WARN(Glib::ustring::compose("%1 nothing to do", players[i]->name));
					Move::ptr move_tactic(new Move(players[i], the_world));
					move_tactic->set_position(players[i]->position());
					tactics[i] = move_tactic;
				} else if (order[w] < block_targets.size()){
					Block::ptr block_tactic(new Block(players[i], the_world));
					block_tactic->set_target(block_targets[order[w]]);
					tactics[i] = block_tactic;
				}
				else{
					Move::ptr move_tactic(new Move(players[i], the_world));
					move_tactic->set_position(waypoints[order[w]]);
					tactics[i] = move_tactic;
				}
				++w;
			}
		}
		else {
			size_t w = 0;
			for (size_t i = 1; i < players.size(); ++i) {
				if (w >= waypoints.size()) {
					LOG_WARN(Glib::ustring::compose("%1 nothing to do", players[i]->name));
					Move::ptr move_tactic(new Move(players[i], the_world));
					move_tactic->set_position(players[i]->position());
					tactics[i] = move_tactic;
				} else {
					Move::ptr move_tactic(new Move(players[i], the_world));
					move_tactic->set_position(waypoints[order[w]]);
					tactics[i] = move_tactic;
				}
				++w;
			}
		}
		
		{
			Shoot::ptr shoot_tactic = Shoot::ptr(new Shoot(players[0], the_world));
			if (OFFENSIVE_PIVOT) shoot_tactic->set_pivot(false);
			tactics[0] = shoot_tactic;
		}
	}

	unsigned int flags = AIFlags::calc_flags(the_world->playtype());

	for (size_t i = 0; i < tactics.size(); ++i) {
		if (static_cast<int>(i) == baller) {
			tactics[i]->set_flags(flags | AIFlags::CLIP_PLAY_AREA);
		} else {
			tactics[i]->set_flags(flags);
		}
		tactics[i]->tick();
	}

}

void Offensive::players_changed() {
}

