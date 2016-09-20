#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/gradient_approach/optimizepass.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "ai/hl/stp/world.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>


using namespace AI::HL::W;
using namespace AI::HL::STP::GradientApproach;

PassInfo::PassInfo() {
	// This constructor is only ever called once.
	// PassLoop thread initiated when singleton created.

	//GradientApproach::passMainLoop * passLoop;
	//std::thread pass_thread(passLoop.superLoop);
}

PassInfo & PassInfo::Instance() {
        // Since it's a static variable, if the class has already been created,
        // It won't be created again. Supposed to be thread safe in C++11.
       
        static PassInfo myInstance;
        // Return a reference to our instance.
        return myInstance;
}

void PassInfo::setThreadRunning(bool new_val){
	thread_running_mutex.lock();
	thread_running = new_val;
	thread_running_mutex.unlock();
}

bool PassInfo::threadRunning(){
	thread_running_mutex.lock();
	bool return_val = thread_running;
	thread_running_mutex.unlock();
	return return_val;
}


std::vector<PassInfo::passDataStruct> PassInfo::getCurrentPoints(){
    currentPoints_mutex.lock();
    std::vector<PassInfo::passDataStruct> return_val = currentPoints;
    currentPoints_mutex.unlock();
    return return_val;
}

void PassInfo::updateCurrentPoints(std::vector<PassInfo::passDataStruct> newPoints){
	currentPoints_mutex.lock();
	currentPoints = newPoints;
	currentPoints_mutex.unlock();
}


void PassInfo::updateWorldSnapshot(World world){
	worldSnapshot new_snapshot;

	// in case we want another point as the passer other than the one closest to the ball
	if(tacticInfo.use_stored_point_as_passer){
		new_snapshot.passer_position = tacticInfo.alt_passer_point;
		new_snapshot.passer_orientation = tacticInfo.alt_passer_orientation;

		Player alt_passer = *std::min_element(world.friendly_team().begin(), world.friendly_team().end(), AI::HL::Util::CmpDist<Player>(tacticInfo.alt_passer_point));
		Player goalie =  *std::min_element(world.friendly_team().begin(), world.friendly_team().end(), AI::HL::Util::CmpDist<Player>(world.field().friendly_goal()));
		for(Player player: world.friendly_team()){

			if(player != alt_passer && player != goalie){
				new_snapshot.passee_positions.push_back(player.position());
				new_snapshot.passee_velocities.push_back(player.velocity());
			}
		}
	}

	// otherwise determine through clac_fastest_grab_ball_destt
	else {
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

		//Player goalie =  *std::min_element(world.friendly_team().begin(), world.friendly_team().end(), AI::HL::Util::CmpDist<Player>(world.field().friendly_goal()));

		for(Player player: world.friendly_team()){
			if(player.position() != passer.position()){
				new_snapshot.passee_positions.push_back(player.position());
				new_snapshot.passee_velocities.push_back(player.velocity());
			}
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
	
	{
		std::lock_guard<std::mutex> lock(world_mutex);
		snapshot = new_snapshot;
	}
}

PassInfo::worldSnapshot PassInfo::getWorldSnapshot() {
	world_mutex.lock();
	PassInfo::worldSnapshot return_val = snapshot;
	world_mutex.unlock();
	return return_val;
}
