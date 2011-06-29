#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"

using namespace AI::HL::STP;

// ShootData EvaluateShoot::compute(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) const {

namespace {
	DoubleParam reduced_radius("reduced radius for calculating best shot (robot radius ratio)", "STP/Action/shoot", 0.8, 0.0, 1.0);
}

DoubleParam AI::HL::STP::Evaluation::shoot_accuracy("Angle threshold (in degrees) that defines shoot accuracy, bigger is more accurate", "STP/shoot", 0.0, -180.0, 180.0);

Evaluation::ShootData Evaluation::evaluate_shoot(const World &world, Player::CPtr player) {
	ShootData data;

	std::pair<Point, double> shot = AI::HL::Util::calc_best_shot(world, player);
	data.reduced_radius = false;

	if (shot.second == 0) {
		shot = AI::HL::Util::calc_best_shot(world, player, reduced_radius);
		data.reduced_radius = true;
	}

	double ori = (shot.first - player->position()).orientation();
	double ori_diff = angle_diff(ori, player->orientation());
	data.accuracy_diff = radians2degrees(ori_diff - (shot.second / 2));

	if (player->kicker_directional()) {
		data.accuracy_diff -= 45;
	}

	data.target = shot.first;
	data.angle = shot.second;
	data.can_shoot = (data.accuracy_diff < -shoot_accuracy);
	data.blocked = (shot.second == 0);

#warning a fix to other parts of the code for now
	if (data.blocked) {
		data.target = world.field().enemy_goal();
	}

	return data;
}

Evaluation::ShootData Evaluation::evaluate_shoot_target(const World &world, Player::CPtr player, const Point target) {
	ShootData data;

	std::pair<Point, double> shot = AI::HL::Util::calc_best_shot_target(world, target, player);
	data.reduced_radius = false;

	if (shot.second == 0) {
		shot = AI::HL::Util::calc_best_shot_target(world, target, player, reduced_radius);
		data.reduced_radius = true;
	}

	double ori = (shot.first - player->position()).orientation();
	double ori_diff = angle_diff(ori, player->orientation());
	data.accuracy_diff = radians2degrees(ori_diff - (shot.second / 2));

	if (player->kicker_directional()) {
		data.accuracy_diff -= 45;
	}

	data.target = shot.first;
	data.angle = shot.second;
	data.can_shoot = (data.accuracy_diff < -shoot_accuracy);
	data.blocked = (shot.second == 0);

#warning a fix to other parts of the code for now
	if (data.blocked) {
		data.target = world.field().enemy_goal();
	}

	return data;
}

bool Evaluation::can_shoot_target(const World &world, Player::CPtr player, const Point target, bool pass) {
	AI::HL::Util::calc_best_shot_target(world, target, player, pass).second > AI::HL::Util::shoot_accuracy * M_PI / 180.0;
}

