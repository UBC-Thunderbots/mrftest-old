#include "ai/hl/stp/evaluation/team.h"

using namespace AI::HL::STP;

Player AI::HL::STP::Evaluation::nearest_friendly(World world, Point target) {
	Player plr;
	double dist = 0;

	for (const Player i : world.friendly_team()) {
		double d = (target - i.position()).len();
		if (!plr || d < dist) {
			plr = i;
			dist = d;
		}
	}

	return plr;
}

Robot AI::HL::STP::Evaluation::nearest_enemy(World world, Point target) {
	Robot bot;
	double dist = 0;

	for (const Robot i : world.enemy_team()) {
		double d = (target - i.position()).len();
		if (!bot || d < dist) {
			bot = i;
			dist = d;
		}
	}

	return bot;
}

