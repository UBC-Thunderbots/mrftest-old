#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::ShootData;
using AI::HL::STP::Evaluation::evaluate_shoot;

//ShootData EvaluateShoot::compute(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) const {

ShootData AI::HL::STP::Evaluation::evaluate_shoot(const AI::HL::W::World &world, AI::HL::W::Player::CPtr player) {
	ShootData data;

	std::pair<Point, double> shot = AI::HL::Util::calc_best_shot(world, player);

	data.target = shot.first;
	data.angle = shot.second;
	data.can_shoot = (shot.second > AI::HL::Util::shoot_accuracy);
	data.ball_on_front = false;
	data.ball_visible = false;

	return data;
}

