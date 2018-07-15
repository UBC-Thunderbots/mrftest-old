#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/gradient_approach/optimizepass.h"
#include "ai/hl/stp/gradient_approach/ratepass.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/gradient_approach/optimizepass.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace AI::HL::W;
using namespace AI::HL::STP::GradientApproach;

PassInfo::PassInfo()
{
    // This constructor is only ever called once.
    // PassLoop thread initiated when singleton created.

	//GradientApproach::passMainLoop * passLoop;
	pass_thread = std::thread(GradientApproach::superLoop);//passLoop.superLoop);
	std::cout << "Made Pass Info singleton" << std::endl;
}

PassInfo & PassInfo::Instance() {
        // Since it's a static variable, if the class has already been created,
        // It won't be created again. Supposed to be thread safe in C++11.
       
        static PassInfo myInstance;
        // Return a reference to our instance.
        return myInstance;
}
/*
void PassInfo::setThreadRunning(bool new_val){
	thread_running_mutex.lock();
	thread_running = new_val;
	thread_running_mutex.unlock();
}

void PassInfo::setThreadRunning(bool new_val)
{
    thread_running_mutex.lock();
    thread_running = new_val;
    thread_running_mutex.unlock();
}
*/

std::vector<PassInfo::passDataStruct> PassInfo::getCurrentPoints()
{
    currentPoints_mutex.lock();
    std::vector<PassInfo::passDataStruct> return_val = currentPoints;
    currentPoints_mutex.unlock();
    return return_val;
}

PassInfo::passDataStruct PassInfo::getBestPass(){
	//std::cout << "Getting best pass" << std::endl << std::flush;
    currentPoints_mutex.lock();
	//std::cout << "pass list length: " << currentPoints.size() << std::endl << std::flush;;
	if(currentPoints.size() == 0){
		std::cout << "no pass objects" << std::endl << std::flush;
    	currentPoints_mutex.unlock();
		return(PassInfo::passDataStruct(4.0, 0.0, 0.5, 4.0, 0.5));
	}
	PassInfo::passDataStruct best_pass = *std::max_element(currentPoints.begin(),currentPoints.end(),
														[] (GradientApproach::PassInfo::passDataStruct lhs,
														GradientApproach::PassInfo::passDataStruct rhs) {return lhs.quality < rhs.quality;});

    currentPoints_mutex.unlock();
	//std::cout << "Finished best pass" << std::endl << std::flush;;
    return best_pass;
}

double PassInfo::ratePass(PassInfo::passDataStruct pass){
	worldSnapshot snap = getWorldSnapshot();
	return GradientApproach::ratePass(snap, Point(pass.params.at(0), pass.params.at(1)), pass.params.at(2), pass.params.at(3));
}

void PassInfo::updateCurrentPoints(std::vector<PassInfo::passDataStruct> newPoints){
	currentPoints_mutex.lock();
	currentPoints = newPoints;
	currentPoints_mutex.unlock();
}


PassInfo::worldSnapshot PassInfo::convertToWorldSnapshot(World world){
	worldSnapshot new_snapshot;
	std::set<Player> players;
	for (const Player p : world.friendly_team()) {
		players.insert(p);
	}
    // should this be calculated differently?
	Player goalie =  *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(world.field().friendly_goal()));

	// in case we want another point as the passer other than the one closest to the ball
	Player passer;
	if(use_alt_passer){
		std::lock_guard<std::mutex> lock(alt_passer_mutex);
		new_snapshot.passer_position = alt_point;
		new_snapshot.passer_orientation = alt_ori;
		passer = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(alt_point));
	}else {
	// otherwise determine through clac_fastest_grab_ball_destt
		Player passer;
		double min_dist = 1e99;
		for (Player player : world.friendly_team()) {
			Point dest = Evaluation::calc_fastest_grab_ball_dest(world, player);
			if (!passer || min_dist > (dest - player.position()).len()) {
				min_dist = (dest - player.position()).len();
				passer = player;
			}
		}
		new_snapshot.passer_position = passer.position();
		new_snapshot.passer_orientation = passer.orientation();
	}
	for(Player player: world.friendly_team()){
		if(player != passer && player != goalie){
			new_snapshot.passee_positions.push_back(player.position());
			new_snapshot.passee_velocities.push_back(player.velocity());
		}
	}
	
	for(Robot robot : world.enemy_team()){
		new_snapshot.enemy_positions.push_back(robot.position());
		new_snapshot.enemy_velocities.push_back(robot.velocity());
	}

	new_snapshot.enemy_goal_boundary = world.field().enemy_goal_boundary();
	new_snapshot.friendly_goal_boundary = world.field().friendly_goal_boundary();
	new_snapshot.field_width = world.field().width();
	new_snapshot.field_length = world.field().length();
	new_snapshot.enemy_goal = world.field().enemy_goal();
	new_snapshot.friendly_goal = world.field().friendly_goal();

	return new_snapshot;
}

void PassInfo::updateWorldSnapshot(PassInfo::worldSnapshot new_snapshot){
	std::lock_guard<std::mutex> lock(world_mutex);
	snapshot = new_snapshot;
}

PassInfo::worldSnapshot PassInfo::getWorldSnapshot() {
	std::lock_guard<std::mutex> lock(world_mutex);
	PassInfo::worldSnapshot return_val = snapshot; // copy
	return return_val;
}
void PassInfo::setAltPasser(Point new_point, Angle new_ori) {
	std::lock_guard<std::mutex> lock(alt_passer_mutex);
	alt_point = new_point;
	alt_ori = new_ori;
	use_alt_passer = true;
}
void PassInfo::resetAltPasser() {
	std::lock_guard<std::mutex> lock(alt_passer_mutex);
	use_alt_passer = false;
}
