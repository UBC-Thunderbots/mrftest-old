#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
using namespace AI::HL::STP;

namespace {
	DoubleParam reduced_radius_small("small reduced radius for calculating best shot (robot radius ratio)", "STP/Shoot", 0.4, 0.0, 1.1);

	DoubleParam reduced_radius_big("big reduced radius for calculating best shot (robot radius ratio)", "STP/Shoot", 0.8, 0.0, 1.1);
}

Angle Evaluation::get_shoot_score(const World &world, Player::Ptr player, bool use_reduced_radius) {
	double radius;
	if (use_reduced_radius) {
		radius = reduced_radius_small;
	} else {
		radius = reduced_radius_big;
	}

	std::vector<std::pair<Point, Angle> > openings = AI::HL::Util::calc_best_shot_all(world, player, radius);

	for (std::vector<std::pair<Point, Angle> >::iterator it = openings.begin(); it != openings.end(); ++it) {
		Angle centre_ang = player->orientation();
		Angle ang_1 = (it->first - player->position()).orientation() + it->second / 2.0;
		Angle ang_2 = (it->first - player->position()).orientation() - it->second / 2.0;
		if (ang_1.angle_diff(centre_ang) + ang_2.angle_diff(centre_ang) > it->second + Angle::of_radians(1e-6)) {
			continue;
		}
		return std::min(ang_1.angle_diff(centre_ang), ang_2.angle_diff(centre_ang));
	}
	return Angle::ZERO;
}

/*
   Point Evaluation::get_best_shoot_target(const World &world, Player::Ptr player) {
    //			std::vector<std::pair<Point, double> > calc_best_shot_all(const AI::HL::W::World &world, AI::HL::W::Player::CPtr player, double radius = 1.0);
    std::vector<std::pair<Point, double> > openings = AI::HL::Util::calc_best_shot_all(world, player);

    double ans = 0.0;
    Point post_low(world.field().length()/2.0, -world.field().goal_width()/2.0);
    Point post_high(world.field().length()/2.0, -world.field().goal_width()/2.0);
    Point player_dir(1, 0);
    player_dir = player_dir.rotate(player->orientation());

    if(openings.size() == 0){
        Point ans(world.field().length()/2.0, 0.0);
        return ans;
    }

    std::pair<Point, double> best = *(openings.begin());
    for(std::vector<std::pair<Point, double> >::iterator it = openings.begin(); it!= openings.end(); it++){
        if( it->second > best.second){
            best = *it;
        }

        double centre_ang = (it->first - player->position()).orientation();
        double ang_1 = (it->first - post_low).orientation();
        double ang_2 = (it->first - post_high).orientation();
        double both = angle_diff(ang_1, ang_2);
        if( angle_diff(ang_1, centre_ang) > both || angle_diff(ang_2, centre_ang) > both){
            continue;
        }
        if(it->second > best.second){
            best = *it;
        }
    }
    return best.first;
   }
 */
Evaluation::ShootData Evaluation::evaluate_shoot(const World &world, Player::CPtr player, bool use_reduced_radius) {
	ShootData data;

	double radius;
	if (use_reduced_radius) {
		radius = reduced_radius_small;
	} else {
		radius = reduced_radius_big;
	}

	auto shot = AI::HL::Util::calc_best_shot(world, player, radius);

	data.reduced_radius = true;

	Angle ori = (shot.first - player->position()).orientation();
	Angle ori_diff = ori.angle_diff(player->orientation());
	data.accuracy_diff = ori_diff - (shot.second / 2);

	data.target = shot.first;
	data.angle = shot.second;
	data.can_shoot = data.accuracy_diff < -shoot_accuracy;
	data.blocked = shot.second == Angle::ZERO;

#warning a fix to other parts of the code for now
	if (data.blocked) {
		data.target = world.field().enemy_goal();
	}

	return data;
}

