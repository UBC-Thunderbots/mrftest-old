#include "ai/hl/stp/module/shoot.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;
using AI::HL::STP::Module::ShootStats;
using AI::HL::STP::Module::EvaluateShoot;

ShootStats EvaluateShoot::compute(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) const {
	ShootStats stats;

	std::pair<Point, double> shot = AI::HL::Util::calc_best_shot(world, player);

	stats.target = shot.first;
	stats.angle = shot.second;
	stats.good = (shot.second > AI::HL::Util::shoot_accuracy);

	return stats;
}

