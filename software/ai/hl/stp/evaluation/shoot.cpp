#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
//#include "geom/angle.h"
using namespace AI::HL::STP;

namespace {
	DoubleParam reduced_radius("reduced radius for calculating best shot (robot radius ratio)", "STP/Shoot", 0.8, 0.0, 1.0);
}


double Evaluation::get_shoot_score(const World &world, Player::Ptr player) {
	//			std::vector<std::pair<Point, double> > calc_best_shot_all(const AI::HL::W::World &world, AI::HL::W::Player::CPtr player, double radius = 1.0);
	std::vector<std::pair<Point, double> > openings = AI::HL::Util::calc_best_shot_all(world, player);

	double ans = 0.0;
	Point post_low(world.field().length()/2.0, -world.field().goal_width()/2.0);
	Point post_high(world.field().length()/2.0, -world.field().goal_width()/2.0);
	Point player_dir(1, 0);
	player_dir = player_dir.rotate(player->orientation());

	std::pair<Point, double> best = *(openings.begin());
	for(std::vector<std::pair<Point, double> >::iterator it = openings.begin(); it!= openings.end(); it++){
		double centre_ang = (it->first - player->position()).orientation();
		double ang_1 = (it->first - post_low).orientation();
		double ang_2 = (it->first - post_high).orientation();
		double both = angle_diff(ang_1, ang_2);
		if( angle_diff(ang_1, centre_ang) > both || angle_diff(ang_2, centre_ang) > both){
			continue;
		}
		return std::min(angle_diff(ang_1, centre_ang), angle_diff(ang_2, centre_ang));
	}
	return 0.0;
}


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
