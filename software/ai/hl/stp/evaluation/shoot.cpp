#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::ShootData;
using AI::HL::STP::Evaluation::evaluate_shoot;

// ShootData EvaluateShoot::compute(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) const {

namespace {
	DoubleParam reduced_radius("reduced radius for calculating best shot (robot radius ratio)", "STP/Action/shoot", 0.8, 0.0, 1.0);
}

DoubleParam AI::HL::STP::Evaluation::shoot_accuracy("Shoot Accuracy (degrees)", "STP/shoot", 20.0, 0, 90.0);

ShootData AI::HL::STP::Evaluation::evaluate_shoot(const AI::HL::W::World &world, AI::HL::W::Player::CPtr player) {
	ShootData data;

	std::pair<Point, double> shot = AI::HL::Util::calc_best_shot(world, player);

	if (shot.second == 0) {
		shot = AI::HL::Util::calc_best_shot(world, player, reduced_radius);
	}

	double ori = (target.first - player->position()).orientation();
	double ori_diff = angle_diff(ori, player->orientation());
	data.allowance = radians2degrees(target.second - 2 * ori_diff);

	data.target = shot.first;
	data.angle = shot.second;
	data.can_shoot = data.allowance < shoot_accuracy;
	data.blocked = (shot.second == 0);
	data.ball_on_front = false;
	data.ball_visible = false;

#warning a fix to other parts of the code for now
	if (data.blocked) {
		data.target = world.field().enemy_goal();
	}

	return data;
}

