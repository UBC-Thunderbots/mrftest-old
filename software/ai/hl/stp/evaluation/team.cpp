#include "ai/hl/stp/evaluation/team.h"

using namespace AI::HL::STP;

Player::CPtr AI::HL::STP::Evaluation::nearest_friendly(World world, Point target) {
	Player::CPtr plr;
	double dist = 0;

	for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
		double d = (target - world.friendly_team().get(i)->position()).len();
		if (!plr || d < dist) {
			plr = world.friendly_team().get(i);
			dist = d;
		}
	}

	return plr;
}

Robot::Ptr AI::HL::STP::Evaluation::nearest_enemy(World world, Point target) {
	Robot::Ptr bot;
	double dist = 0;

	for (std::size_t i = 0; i < world.enemy_team().size(); i++) {
		double d = (target - world.enemy_team().get(i)->position()).len();
		if (!bot || d < dist) {
			bot = world.enemy_team().get(i);
			dist = d;
		}
	}

	return bot;
}

