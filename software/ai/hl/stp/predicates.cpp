#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"

#include <set>

using namespace AI::HL::STP;
using namespace AI::HL::W;

bool AI::HL::STP::Predicates::goal(const World &) {
	return false;
}

bool AI::HL::STP::Predicates::playtype(const World &world, AI::Common::PlayType playtype) {
	return world.playtype() == playtype;
}

bool AI::HL::STP::Predicates::our_ball(const World &world) {
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (friendly.get(i)->has_ball()) {
			return true;
		}
	}
	return false;
}

bool AI::HL::STP::Predicates::their_ball(const World &world) {
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (AI::HL::Util::posses_ball(world, enemy.get(i))) {
			return true;
		}
	}
	return false;
}

bool AI::HL::STP::Predicates::none_ball(const World &world) {
	return !our_ball(world) && !their_ball(world);
}

bool AI::HL::STP::Predicates::our_team_size_at_least(const World &world, const unsigned int n) {
	return world.friendly_team().size() >= n;
}

bool AI::HL::STP::Predicates::their_team_size_at_least(const World &world, const unsigned int n) {
	return world.enemy_team().size() >= n;
}

bool AI::HL::STP::Predicates::their_team_size_at_most(const World &world, const unsigned int n) {
	return world.enemy_team().size() <= n;
}

bool AI::HL::STP::Predicates::ball_x_less_than(const World &world, const double x) {
	return world.ball().position().x < x;
}

bool AI::HL::STP::Predicates::ball_x_greater_than(const World &world, const double x) {
	return world.ball().position().x > x;
}

bool AI::HL::STP::Predicates::ball_on_our_side(const World &world) {
	return world.ball().position().x <= 0;
}

bool AI::HL::STP::Predicates::ball_on_their_side(const World &world) {
	return world.ball().position().x > 0;
}

bool AI::HL::STP::Predicates::ball_in_our_corner(const World &world) {
	return world.ball().position().x <= -world.field().length()/4 && std::fabs(world.ball().position().y) > world.field().goal_width();
}

bool AI::HL::STP::Predicates::ball_in_their_corner(const World &world) {
	return world.ball().position().x >= world.field().length()/4 && std::fabs(world.ball().position().y) > world.field().goal_width();
}

bool AI::HL::STP::Predicates::ball_midfield(const World &world){
	return std::fabs(world.ball().position().x) < world.field().length()/4;
}

bool AI::HL::STP::Predicates::baller_can_shoot(const World &world){
	const FriendlyTeam &friendly = world.friendly_team();
	std::set<Player::CPtr> players;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		players.insert(friendly.get(i));
	}
	const Player::CPtr baller = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::CPtr>(world.ball().position()));
	return AI::HL::Util::calc_best_shot(world, baller).second > AI::HL::Util::shoot_accuracy * M_PI / 180.0;
}

