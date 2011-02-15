#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;
using namespace AI::HL::W;

bool AI::HL::STP::Predicates::goal(World &) {
	return false;
}

bool AI::HL::STP::Predicates::playtype(World &world, const PlayType::PlayType playtype) {
	return world.playtype() == playtype;
}

bool AI::HL::STP::Predicates::our_ball(World &world) {
	FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (friendly.get(i)->has_ball()) {
			return true;
		}
	}
	return false;
}

bool AI::HL::STP::Predicates::their_ball(World &world) {
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (AI::HL::Util::posses_ball(world, enemy.get(i))) {
			return true;
		}
	}
	return false;
}

bool AI::HL::STP::Predicates::none_ball(World &world) {
	return !our_ball(world) && !their_ball(world);
}

bool AI::HL::STP::Predicates::our_team_size_at_least(World &world, const unsigned int n) {
	return world.friendly_team().size() >= n;
}

bool AI::HL::STP::Predicates::their_team_size_at_least(World &world, const unsigned int n) {
	return world.enemy_team().size() >= n;
}

bool AI::HL::STP::Predicates::their_team_size_at_most(World &world, const unsigned int n) {
	return world.enemy_team().size() <= n;
}

bool AI::HL::STP::Predicates::ball_x_less_than(World &world, const double x) {
	return world.ball().position().x < x;
}

bool AI::HL::STP::Predicates::ball_x_greater_than(World &world, const double x) {
	return world.ball().position().x > x;
}

bool AI::HL::STP::Predicates::ball_on_our_side(World &world) {
	return world.ball().position().x <= 0;
}

bool AI::HL::STP::Predicates::ball_on_their_side(World &world) {
	return world.ball().position().x > 0;
}

