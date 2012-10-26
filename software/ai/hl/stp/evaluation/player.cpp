#include "ai/hl/stp/evaluation/player.h"

using namespace AI::HL::STP;

bool Evaluation::player_within_angle_thresh(const Point position, const Angle orientation, const Point target, Angle threshold) {
	Point pass_dir = (target - position).norm();
	Point facing_dir = Point(1, 0).rotate(orientation);
	// double dir_thresh = cos((threshold*M_PI / 180.0));
	double dir_thresh = threshold.cos();
	return facing_dir.dot(pass_dir) > dir_thresh;
}

bool Evaluation::player_within_angle_thresh(Player player, const Point target, Angle threshold) {
	return player_within_angle_thresh(player->position(), player->orientation(), target, threshold);
}

