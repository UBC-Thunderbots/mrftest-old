#include "autoref/autoref.h"
#include "ai/hl/world.h"
#include "ai/backend/ball.h"
#include "ai/backend/backend.h"
#include "ai/hl/stp/predicates.h"
#include "log/shared/enums.h"
#include "log/shared/magic.h"
#include "util/algorithm.h"
#include "geom/point.h"
#include "util/annunciator.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/param.h"
#include "util/timestep.h"
#include <iostream>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <locale>
#include <ratio>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/ustring.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>

using AI::Common::PlayType;
using AI::HL::STP::Predicates::playtype;


AI::AutoRef* AI::AutoRef::instance = nullptr;
//define the velocity of what constitutes a violent collision.
//If the collision message is being printed too much, raise this value.
//If the collision message is not being printed enough, lower this value.
#define VIOLENT_VELOCITY 3.0
//this offset is used for determining if robots are next to one another
//if the autrof thinks that they are together when they are not, raise this value
//If the autoref think that they are not together when they are, lower this value.
#define ROBOT_OFFSET 0.02

AI::AutoRef::AutoRef(const AI::AIPackage &ai, const AI::HL::W::World &world) : ai(ai), world(world), FT(world.friendly_team()), ET(world.enemy_team()),ball_is_out_of_play(
		false), print_ball_out_of_play(true), last_touch(NO_TEAM), enemy_in_defense_circle(false), friendly_in_defense_circle(false),partial_violation_e(false), full_violation_e(false), partial_violation_f(false),
		full_violation_f(false), print_partial_violation_f(false), print_full_violation_f(false),print_partial_violation_e(false), print_full_violation_e(false),throwaway(0), print_ball_velocity(true),
		velocity_greater_than_eight(false),print_velocity_message(false), print_last_team_to_touch_ball(false), verbose(false), update_enemy_past(false),update_friendly_past(false){

	//connect to the signals that fire when receiving packets
	ai.backend.signal_vision().connect(sigc::mem_fun(this, &AI::AutoRef::on_vision_packet));
	ai.backend.signal_refbox().connect(sigc::mem_fun(this, &AI::AutoRef::on_refbox_packet));

	//connect to the software tick signal
	ai.backend.signal_tick().connect(sigc::mem_fun(this, &AI::AutoRef::tick));
}

AI::AutoRef* AI::AutoRef::getInstance(const AI::AIPackage &ai, const AI::HL::W::World &world){
	if(!instance){
		instance = new AutoRef(ai, world);
	}
	return instance;
}



AI::AutoRef::~AutoRef(){

}

void AI::AutoRef::tick(){

	//update the past positions once every tick
	update_enemy_past = false;
	update_friendly_past = false;

	//throwaway first 16 packets.
	//why not
	while(throwaway<16){

		//15 packets in and we should have the following data initialized to resize these vectors
		if(throwaway==15){
			FP.resize(12);
			EP.resize(12);
		}
		throwaway++;
		return;
	}


	/*print if ball velocity is greater than 8 when true*/
	print_ball_velocity = true;

	/*print if ball has left field and by which team*/
	ball_has_left();

	/*print if multiple defender rule is violated*/
	multiple_defenders();

	/*print if a violent collision has occured*/
	violent_collision();

	/*print if a robot is pushing against another without a collision*/
	//this is actually done within analyze_the_past()


}

//look at the past and see which team collided with the other team.
//Possible return values are NO_TEAM, FRIEND_TEAM, ENEMY_TEAM
AI::Team AI::AutoRef::analyze_the_past(unsigned int id_f, unsigned int id_e){

	double avg_velocity_e = 0;
	double avg_velocity_f = 0;
	double distance_1;
	double distance_2;
	double time_1;
	double time_2;
	double velocity_1;
	double velocity_2;

	//calculate the average velocity that the robot moved over the last max(i) data points
	for(int i = 0; i<2;i++){

		distance_1 = std::sqrt(std::pow(FP[id_f].at(i).first.x-FP[id_f].at(i+1).first.x,2)+std::pow(FP[id_f].at(i).first.y-FP[id_f].at(i+1).first.y,2)) ;
		distance_2 = std::sqrt(std::pow(EP[id_e].at(i).first.x-EP[id_e].at(i+1).first.x,2)+std::pow(EP[id_e].at(i).first.y-EP[id_e].at(i+1).first.y,2)) ;

		time_1 = double((FP[id_f].at(i).second - FP[id_f].at(i+1).second).count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
		time_2 = double((EP[id_e].at(i).second - EP[id_e].at(i+1).second).count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;


		velocity_1 = distance_1 /time_1;
		velocity_2 = distance_2 /time_2;
		avg_velocity_e = avg_velocity_e + velocity_2;
		avg_velocity_f = avg_velocity_f + velocity_1;
	}

	//I actually have no idea what the velocity of a violent collision is
	if(avg_velocity_f>VIOLENT_VELOCITY){
		printf("Friendly team has collided with Enemy team\n");
		return FRIENDLY_TEAM;
	}else if(avg_velocity_e>VIOLENT_VELOCITY){
		printf("Enemy Team has collided with Friendly Team\n");
		return ENEMY_TEAM;
	}else{

		/*This is for determining if robots are pushing against each other */

		//calculate distance between friendly robot in newest time frame and the enemy robot in the next newest time frame.
		//and calculate the distance between enemy robot in newest time frame and the friendly robot in the next newest time frame.
		double d_f_e = std::sqrt(std::pow(FP[id_f].at(0).first.x - EP[id_e].at(1).first.x,2) + std::pow(FP[id_f].at(0).first.y - EP[id_e].at(1).first.y,2));
		double d_e_f = std::sqrt(std::pow(FP[id_f].at(1).first.x - EP[id_e].at(0).first.x,2) + std::pow(FP[id_f].at(1).first.y - EP[id_e].at(0).first.y,2));

		//if the distance between the current friendly robot and the last enemy robot is farther than the current enemy is to the last friendly
		//then this means that the current enemy robot is closer to the position of the last friendly robot ie, it pushed the friendly robot out of the way
		if(d_f_e > d_e_f && (std::fabs(d_f_e-d_e_f)>0.04)){

			printf("Enemy Robot is pushing against Friendly Robot\n");
		}else if(d_f_e < d_e_f && (std::fabs(d_f_e-d_e_f)>0.04)){

			printf("Friendly Robot is pushing against Enemy Robot\n");


		}

	}


	return NO_TEAM;


}


void AI::AutoRef::violent_collision() {

	AI::Common::TeamIterator<AI::HL::W::Player, AI::BE::Player> FTI = FT.begin();

	//Sweet Jesus of Nazareth its an n^2 algorithms :o
	//dont hate
	for (unsigned int i = 0; i < FT.size(); i++) {

		std::pair<Point, AI::Timestamp> fp;
		AI::HL::W::Robot current_f = *FTI;

		AI::Common::TeamIterator<AI::HL::W::Robot, AI::BE::Robot> ETI = ET.begin();
		for(unsigned int j = 0; j<ET.size(); j++){
			std::pair<Point, AI::Timestamp> ep;
			AI::HL::W::Robot current_e = *ETI;

			//calcualte distance between robots
			double distance = std::sqrt( std::pow(current_f.position().x - current_e.position().x, 2) + std::pow(current_f.position().y - current_e.position().y, 2)  );


			if(!update_enemy_past){
				ep.first = current_e.position();
				ep.second = std::chrono::steady_clock::now();
				assert(current_e.pattern() < EP.size());
				EP[current_e.pattern()].push_front(ep);
				EP[current_e.pattern()].resize(10);
			}

			if(distance <= 2*current_f.MAX_RADIUS + ROBOT_OFFSET){
				//These robots are touching so check if they collided violently by looking at past data
				analyze_the_past(current_f.pattern(), current_e.pattern());
			}

			ETI++;
		}

		update_enemy_past = true;

		if(!update_friendly_past){
			fp.first = current_f.position();
			fp.second = std::chrono::steady_clock::now();
			FP[current_f.pattern()].push_front(fp);
			FP[current_f.pattern()].resize(10);
		}

		FTI++;
	}

	update_friendly_past = true;
}

bool AI::AutoRef::right_side_of_field(double x, Team t) {
	if(t == FRIENDLY_TEAM) {
		if(world.field().friendly_goal().x > 0){
			return (x>0 );
		}
		else if (world.field().friendly_goal().x < 0){
			return (x<0);
		}
	}
	if(t == ENEMY_TEAM) {
		if(world.field().friendly_goal().x > 0) {
			return x < 0;
		}
		else if (world.field().friendly_goal().x < 0) {
			return x > 0;
		}
	}
	return false;
}


void AI::AutoRef::multiple_defenders() {

	//formulate points for the centers of the top and bottom arc.
	//the centers for the defense side lie on the goal line and (0.5*defense area stretch) up and down from the center of the goal
	//If the centres lie on (x,y) on the friendly side, then the centers on the enemy side are at (-x,y)
	Point friendly_centre_of_goal = world.field().friendly_goal();
	Point* friendly_centre_of_top_arc = new Point(friendly_centre_of_goal.x,
			friendly_centre_of_goal.y
					+ world.field().defense_area_stretch()/2);
	Point* friendly_centre_of_bottom_arc = new Point(
			friendly_centre_of_goal.x,
			friendly_centre_of_goal.y
					- world.field().defense_area_stretch()/2);

	Point* enemy_centre_of_top_arc = new Point(- friendly_centre_of_top_arc->x,friendly_centre_of_top_arc->y);
	Point* enemy_centre_of_bottom_arc = new Point(- friendly_centre_of_bottom_arc->x,friendly_centre_of_bottom_arc->y);

	//iteratre through the enemy robots to see if they violate the rule
	AI::Common::TeamIterator<AI::HL::W::Robot, AI::BE::Robot> ETI = ET.begin();

	enemy_in_defense_circle = false;
	partial_violation_e = false;
	full_violation_e = false;

	for (unsigned int i = 0; i < ET.size(); i++) {
		AI::HL::W::Robot current = *ETI;

		//goalie doesn't count
		if(current.pattern() == world.enemy_team().goalie()){
			ETI++;
			continue;
		}

		//this case for if the robot may be in the top arc
		if ((current.position().y > enemy_centre_of_top_arc->y)
				& right_side_of_field(current.position().x, ENEMY_TEAM)) {
			//distance from centre of arc to centre of robot
			double distance = std::sqrt(
					std::pow(current.position().x - enemy_centre_of_top_arc->x, 2)+ std::pow(current.position().y - enemy_centre_of_top_arc->y,2));

			if(distance<=world.field().defense_area_radius() - current.MAX_RADIUS)
			{
				enemy_in_defense_circle = true;
				full_violation_e = true;

			}
			else if( (distance < (world.field().defense_area_radius() + current.MAX_RADIUS))&&
					(distance > (world.field().defense_area_radius() - current.MAX_RADIUS)))
			{
				enemy_in_defense_circle = true;
				partial_violation_e = true;

			}

		}
		//this case for if the robot may be in the bottom arc
		else if ((current.position().y < enemy_centre_of_bottom_arc->y)
				&& right_side_of_field(current.position().x, ENEMY_TEAM))
		{
			double distance =std::sqrt(std::pow(current.position().x - enemy_centre_of_bottom_arc->x,2)
									+ std::pow( current.position().y - enemy_centre_of_bottom_arc->y,2));

			if(distance<world.field().defense_area_radius() - current.MAX_RADIUS)
			{
				enemy_in_defense_circle = true;
				full_violation_e = true;

			}
			else if ((distance < world.field().defense_area_radius()+ current.MAX_RADIUS)&&
					(distance > world.field().defense_area_radius() - current.MAX_RADIUS))
			{
				enemy_in_defense_circle = true;
				partial_violation_e = true;
			}
		}
		//this case for if the robot is in between the arcs
		else if ((current.position().y > enemy_centre_of_bottom_arc->y)
				& (current.position().y < enemy_centre_of_top_arc->y)
				& right_side_of_field(current.position().x, ENEMY_TEAM))
		{
			if (fabs(world.field().length() / 2 - fabs(current.position().x))<= world.field().defense_area_radius() - current.MAX_RADIUS)
			{
				enemy_in_defense_circle = true;
				full_violation_e = true;

			}
			else if (fabs(world.field().length() / 2 - fabs(current.position().x))<= world.field().defense_area_radius() + current.MAX_RADIUS)
			{
				enemy_in_defense_circle = true;
				partial_violation_e = true;
			}
		}

		ETI++;
	}

	if(enemy_in_defense_circle && full_violation_e && !print_full_violation_e){
		printf("Robocup rule fully violated by Enemy Robot!\n");
		print_full_violation_e = true;
		print_partial_violation_e = false;

	}
	else if(enemy_in_defense_circle && partial_violation_e && !full_violation_e && !print_partial_violation_e){
		printf("Robocup rule partially violated by Enemy Robot!\n");
		print_full_violation_e = false;
		print_partial_violation_e = true;
	}
	//reset the print statuses if there is no enemy in the defense
	if(!enemy_in_defense_circle ){
		if(print_partial_violation_e || print_full_violation_e)
		{
			printf("Enemy Robots out of penalty area!\n");
		}

		print_partial_violation_e = false;
		print_full_violation_e = false;
	}

	friendly_in_defense_circle = false;
	partial_violation_f = false;
	full_violation_f = false;
	//iterate through the Friendly robots to see if they violate the rule
	AI::Common::TeamIterator<AI::HL::W::Player, AI::BE::Player> FTI = FT.begin();
	for (unsigned int i = 0; i < FT.size(); i++) {
			AI::HL::W::Robot current = *FTI;

			//goalie doesn't count
			if(current.pattern() == world.friendly_team().goalie()){
				FTI++;
				continue;
			}

			//this case for if the robot may be in the top arc
			if ((current.position().y > friendly_centre_of_top_arc->y) & right_side_of_field(current.position().x, FRIENDLY_TEAM))
			{
				double distance = std::sqrt(
						std::pow(current.position().x - friendly_centre_of_top_arc->x, 2)+ std::pow(current.position().y - friendly_centre_of_top_arc->y,2));

				if( (distance < (world.field().defense_area_radius() + current.MAX_RADIUS))&&
											(distance > (world.field().defense_area_radius() - current.MAX_RADIUS)))
				{
					friendly_in_defense_circle = true;
					partial_violation_f = true;
				}
				else if(distance<world.field().defense_area_radius() - current.MAX_RADIUS)
				{
					friendly_in_defense_circle = true;
					full_violation_f = true;
				}

			}
			//this case for if the robot may be in the bottom arc
			else if ((current.position().y < friendly_centre_of_bottom_arc->y)& right_side_of_field(current.position().x, FRIENDLY_TEAM))
			{
				double distance =
						std::sqrt(
								std::pow(current.position().x - friendly_centre_of_bottom_arc->x, 2)
										+ std::pow(current.position().y - friendly_centre_of_bottom_arc->y,2));


				if( (distance < (world.field().defense_area_radius() + current.MAX_RADIUS))&&
										(distance > (world.field().defense_area_radius() - current.MAX_RADIUS)))
				{
					friendly_in_defense_circle = true;
					partial_violation_f = true;
				}
				else if(distance<world.field().defense_area_radius() - current.MAX_RADIUS)
				{
					friendly_in_defense_circle = true;
					full_violation_f = true;
				}
			}

			//this case for if the robot is in between the arcs
			else if ((current.position().y > friendly_centre_of_bottom_arc->y)
						& (current.position().y < friendly_centre_of_top_arc->y)
						& right_side_of_field(current.position().x, FRIENDLY_TEAM))
			{
				if ((fabs(world.field().length() / 2 - fabs(current.position().x))<= world.field().defense_area_radius() - current.MAX_RADIUS)
						& right_side_of_field(current.position().x, FRIENDLY_TEAM))
				{
					friendly_in_defense_circle = true;
					full_violation_f = true;
				}
				else if (fabs(world.field().length() / 2 - fabs(current.position().x))<= world.field().defense_area_radius() + current.MAX_RADIUS)
				{
					friendly_in_defense_circle = true;
					partial_violation_f = true;
				}
			}


			FTI++;
	}

	if(friendly_in_defense_circle && full_violation_f && !print_full_violation_f){
			printf("Robocup rule fully violated by Friendly Robot!\n");
			print_full_violation_f = true;
			print_partial_violation_f = false;

	}
	else if(friendly_in_defense_circle && partial_violation_f && !full_violation_f && !print_partial_violation_f){
			printf("Robocup rule partially violated by Friendly Robot!\n");
			print_full_violation_f = false;
			print_partial_violation_f = true;
	}
	//reset the print statuses if there is no enemy in the defense
	if(!friendly_in_defense_circle ){
			if(print_partial_violation_f || print_full_violation_f)
			{
				printf("Friendly Robots out of penalty area!\n");
			}

			print_partial_violation_f = false;
			print_full_violation_f = false;
	}

}

void AI::AutoRef::display(){
	//ignore this function. it was a flop
	/*
	display_statement.clear();
	display_statement.append("The Ball is out of play [");
	if(ball_is_out_of_play){
		display_statement.append("y]");
	}else
		display_statement.append("n]");
	std::cout << display_statement;
	display_statement.append("--by FriendlyTeam [");
		if(last_touch == FRIENDLY_TEAM){
			display_statement.append("y]");
		}else
			display_statement.append("n]");
		std::cout << display_statement;
	std::cout << std::string(display_statement.length(),'\r');
	//std::cout << std::string(display_statement.length(),' ');
	//std::cout << std::string(display_statement.length(),'\r');
	*/
}

void AI::AutoRef::last_team_to_touch() {

		//iterate through the robots and  if the distance between the ball and the robot is less than
		//the radius of the bot + an offset, then the robot has the ball

		AI::Common::TeamIterator<AI::HL::W::Player, AI::BE::Player> FTI = FT.begin();
		for (unsigned int i = 0; i < FT.size(); i++) {
			AI::HL::W::Player current = *FTI;
			double distance = std::sqrt(
					std::pow(current.position().x - world.ball().position().x,
							2)
							+ std::pow(
									current.position().y
											- world.ball().position().y, 2));
			if (distance <= current.MAX_RADIUS + offset) {
				if(verbose){
					printf(
							"Friendly Robot %d - I have the ball at (%f, %f). Distance to ball is %f\n",
							i, world.ball().position().x, world.ball().position().y,
							distance);
				}
				last_touch = FRIENDLY_TEAM;
			}
			FTI++;
		}

		AI::Common::TeamIterator<AI::HL::W::Robot, AI::BE::Robot> ETI = ET.begin();
		for (unsigned int i = 0; i < ET.size(); i++) {
			AI::HL::W::Robot current = *ETI;
			double distance = std::sqrt(
					std::pow(current.position().x - world.ball().position().x,
							2)
							+ std::pow(
									current.position().y
											- world.ball().position().y, 2));
			if (distance <= (2 * current.MAX_RADIUS) + offset) {
				if(verbose){
					printf(
							"Enemy Robot %d - I have the ball at (%f, %f). Distance to ball is %f!!!\n",
							i, world.ball().position().x, world.ball().position().y,
							distance);
				}
				last_touch = ENEMY_TEAM;
			}
			ETI++;
		}
	}

void AI::AutoRef::ball_has_left() {

		//find out which team touched the ball last
		last_team_to_touch();

		if ((fabs(world.ball().position().y) > world.field().width() / 2)
				|| (fabs(world.ball().position().x) > world.field().length() / 2)) {
			ball_is_out_of_play = true;
			//no need to flood the console with messages

			if (print_ball_out_of_play == false) {
				if (last_touch == FRIENDLY_TEAM){
					printf("Ball was knocked out by friendly team\n");
				}
				else if (last_touch == ENEMY_TEAM){
					printf("Ball was knocked out by enemy team\n");
				}
				else{
					printf("Ball was knocked out by no team\n");
				}

				print_ball_out_of_play = true;
			}

		} else {
			ball_is_out_of_play = false;
			if (print_ball_out_of_play == true) {
				print_ball_out_of_play = false;
				last_touch = NO_TEAM;
				printf("Ball is on the field\n");
			}
		}

	}

void AI::AutoRef::on_vision_packet(AI::Timestamp, const SSL_WrapperPacket &){
	if(print_ball_velocity && ((world.playtype() != PlayType::STOP) || (world.playtype() !=PlayType::HALT)))
	{
		double velocity = world.ball().velocity().len();

		if(velocity>8){
			if(!print_velocity_message){
				print_velocity_message = true;
				velocity_greater_than_eight = true;
				std::cout << "Velocity is greater than 8. Velocity is  " << velocity << "m/s \n";
			}
		}else{
				print_velocity_message = false;
				velocity_greater_than_eight = false;
		}
	}
}

void AI::AutoRef::on_refbox_packet(AI::Timestamp, const SSL_Referee &){
}
