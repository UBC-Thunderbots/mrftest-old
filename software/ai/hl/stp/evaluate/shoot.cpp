#include "ai/hl/stp/evaluate/shoot.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::ShootStats;
// using AI::HL::STP::Evaluation::evaluate_shoot;

//ShootStats EvaluateShoot::compute(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) const {
const ShootStats AI::HL::STP::Evaluation::shoot_stats(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) {
	ShootStats stats;

	std::pair<Point, double> shot = AI::HL::Util::calc_best_shot(world, player);

	stats.target = shot.first;
	stats.angle = shot.second;
	stats.can_shoot = (shot.second > AI::HL::Util::shoot_accuracy);
	stats.ball_on_front = false;
	stats.ball_visible = false;

	return stats;
}

