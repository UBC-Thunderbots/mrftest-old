#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam reduced_radius("reduced radius for calculating best shot (robot radius ratio)", "STP/Shoot", 0.8, 0.0, 1.0);
}

Evaluation::ShootData Evaluation::evaluate_shoot(const World &world, Player::CPtr player) {
	ShootData data;

	auto shot = AI::HL::Util::calc_best_shot(world, player, reduced_radius);
	data.reduced_radius = true;

	double ori = (shot.first - player->position()).orientation();
	double ori_diff = angle_diff(ori, player->orientation());
	data.accuracy_diff = ori_diff - (shot.second / 2);

	if (player->kicker_directional()) {
		data.accuracy_diff -= degrees2radians(45);
	}

	data.target = shot.first;
	data.angle = shot.second;
	data.can_shoot = (data.accuracy_diff < -degrees2radians(shoot_accuracy));
	data.blocked = (shot.second == 0);

#warning a fix to other parts of the code for now
	if (data.blocked) {
		data.target = world.field().enemy_goal();
	}

	return data;
}
