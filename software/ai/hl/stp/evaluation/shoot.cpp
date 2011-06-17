#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"

using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::ShootData;
using AI::HL::STP::Evaluation::evaluate_shoot;

// ShootData EvaluateShoot::compute(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) const {

namespace {
	DoubleParam reduced_radius("reduced radius for calculating best shot (robot radius ratio)", "STP/Action/shoot", 0.8, 0.0, 1.0);
}

DoubleParam AI::HL::STP::Evaluation::shoot_accuracy("Angle threshold (in degrees) that defines shoot accuracy, smaller is more accurate", "STP/shoot", 0.0, -180.0, 180.0);

ShootData AI::HL::STP::Evaluation::evaluate_shoot(const AI::HL::W::World &world, AI::HL::W::Player::CPtr player) {
	ShootData data;

	std::pair<Point, double> shot = AI::HL::Util::calc_best_shot(world, player);
	data.reduced_radius = false;

	if (shot.second == 0) {
		shot = AI::HL::Util::calc_best_shot(world, player, reduced_radius);
		data.reduced_radius = true;
	}

	double ori = (shot.first - player->position()).orientation();
	double ori_diff = angle_diff(ori, player->orientation());
	data.accuracy_diff = ori_diff - (shot.second / 2);

	data.target = shot.first;
	data.angle = shot.second;
	data.can_shoot = (radians2degrees(data.accuracy_diff) < -shoot_accuracy);
	data.blocked = (shot.second == 0);
	data.ball_on_front = false;
	data.ball_visible = false;

#warning a fix to other parts of the code for now
	if (data.blocked) {
		data.target = world.field().enemy_goal();
	}

	return data;
}

