#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"

#include <set>

using namespace AI::HL::STP;

namespace {
	DoubleParam near_thresh("enemy avoidance distance (robot radius)", "STP/predicates", 3.0, 1.0, 10.0);
}

bool Predicates::goal(const World &) {
	return false;
}

bool Predicates::playtype(const World &world, AI::Common::PlayType playtype) {
	return world.playtype() == playtype;
}

bool Predicates::our_ball(const World &world) {
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (Evaluation::possess_ball(world, friendly.get(i))) {
			return true;
		}
	}
	return false;
}

bool Predicates::their_ball(const World &world) {
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (Evaluation::possess_ball(world, enemy.get(i))) {
			return true;
		}
	}
	return false;
}

bool Predicates::none_ball(const World &world) {
	return !our_ball(world) && !their_ball(world);
}

bool Predicates::our_team_size_at_least(const World &world, const unsigned int n) {
	return world.friendly_team().size() >= n;
}

bool Predicates::our_team_size_exactly(const World &world, const unsigned int n) {
	return world.friendly_team().size() == n;
}

bool Predicates::their_team_size_at_least(const World &world, const unsigned int n) {
	return world.enemy_team().size() >= n;
}

bool Predicates::their_team_size_at_most(const World &world, const unsigned int n) {
	return world.enemy_team().size() <= n;
}

bool Predicates::ball_x_less_than(const World &world, const double x) {
	return world.ball().position().x < x;
}

bool Predicates::ball_x_greater_than(const World &world, const double x) {
	return world.ball().position().x > x;
}

bool Predicates::ball_on_our_side(const World &world) {
	return world.ball().position().x <= 0;
}

bool Predicates::ball_on_their_side(const World &world) {
	return world.ball().position().x > 0;
}

bool Predicates::ball_in_our_corner(const World &world) {
	return world.ball().position().x <= -world.field().length() / 4 && std::fabs(world.ball().position().y) > world.field().centre_circle_radius();
}

bool Predicates::ball_in_their_corner(const World &world) {
	return world.ball().position().x >= world.field().length() / 4 && std::fabs(world.ball().position().y) > world.field().centre_circle_radius();
}

bool Predicates::ball_midfield(const World &world) {
	return std::fabs(world.ball().position().x) < world.field().length() / 4;
}

bool Predicates::ball_near_friendly_goal(const World &world){
	return ball_on_our_side(world) && !ball_in_our_corner(world) && !ball_midfield(world);
}

bool Predicates::ball_near_enemy_goal(const World &world){
	return ball_on_their_side(world) && !ball_in_their_corner(world) && !ball_midfield(world);
}

bool Predicates::baller_can_shoot(const World &world) {
	const Player::CPtr baller = Evaluation::calc_friendly_baller(world);
	if (!baller.is() || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	// return Evaluation::evaluate_shoot(world, baller).angle >= degrees2radians(min_shoot_region);
	return Evaluation::evaluate_shoot(world, baller).can_shoot;
}

bool Predicates::baller_can_pass_target(const World &world, const Point target) {
	const Player::CPtr baller = Evaluation::calc_friendly_baller(world);
	if (!baller.is() || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
#warning something is wierd
	return Evaluation::can_pass(world, baller->position(), target);
}

bool Predicates::baller_can_pass(const World &world) {
	const Player::CPtr baller = Evaluation::calc_friendly_baller(world);
	if (!baller.is() || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::select_passee(world).is();
}

bool Predicates::baller_under_threat(const World &world) {
	const Player::CPtr baller = Evaluation::calc_friendly_baller(world);
	if (!baller.is() || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	int enemy_cnt = 0;
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if ((baller->position() - enemies.get(i)->position()).len() <= near_thresh * AI::HL::W::Robot::MAX_RADIUS) {
			enemy_cnt++;
		}
	}

	return enemy_cnt >= 2;
}

bool Predicates::enemy_baller_can_shoot(const World &world) {
	const Robot::Ptr baller = Evaluation::calc_enemy_baller(world);
	if (!baller.is() || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::enemy_can_shoot_goal(world, baller);
}

bool Predicates::enemy_baller_can_pass(const World &world) {
	const Robot::Ptr baller = Evaluation::calc_enemy_baller(world);
	if (!baller.is() || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::calc_enemy_pass(world, baller) > 0;
}

bool Predicates::enemy_baller_can_pass_shoot(const World &world) {
	const Robot::Ptr baller = Evaluation::calc_enemy_baller(world);
	if (!baller.is() || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::calc_enemy_pass(world, baller) > 0 && Evaluation::calc_enemy_pass(world, baller) < 3;
}

bool Predicates::offensive(const World &world) {
	return our_ball(world) || ball_on_their_side(world);
}

bool Predicates::defensive(const World &world) {
	return (their_ball(world) || ball_on_our_side(world)) && !offensive(world);
}

bool Predicates::num_of_enemies_on_our_side_at_least(const World &world, const unsigned int n) {
	unsigned int cnt = 0;
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if (enemies.get(i)->position().x < 0) {
			cnt++;
		}
	}
	if (cnt >= n) return true;
	return false;
}

bool Predicates::ball_inside_region(const World &world, Region region) {
	return region.inside(world.ball().position());
}

bool Predicates::fight_ball(const World &world) {
	return our_ball(world) && their_ball(world);
}

bool Predicates::can_shoot_ray(const World& world) {
	const FriendlyTeam& friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::CPtr player = friendly.get(i);
		if (!Evaluation::possess_ball(world, player)) continue;
		if (Evaluation::best_shoot_ray(world, player).first) {
			return true;
		}
	}
	return false;
}

bool Predicates::ball_inside_robot(const World &world){
	const Point ball = world.ball().position();
	
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if ((friendly.get(i)->position() - ball).len() < AI::HL::Util::POS_CLOSE){
			return true; 
		}
	}
	
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if ((enemies.get(i)->position() - ball).len() < AI::HL::Util::POS_CLOSE) {
			return true;
		}
	}
	return false;
}

