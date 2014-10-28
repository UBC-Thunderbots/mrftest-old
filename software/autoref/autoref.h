#ifndef AUTOREF_H
#define AUTOREF_H

#include "ai/ai.h"
#include "ai/backend/ball.h"
#include "ai/common/field.h"
#include "ai/backend/backend.h"
#include "ai/common/world.h"
#include "ai/common/team.h"
#include "ai/hl/world.h"
#include "proto/log_record.pb.h"
#include "proto/referee.pb.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include "util/signal.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <sigc++/trackable.h>



namespace AI {
enum Team {
 	FRIENDLY_TEAM, ENEMY_TEAM, NO_TEAM
 };


 class AutoRef final : public NonCopyable, public sigc::trackable {


 	 public:
	 	 	 	~AutoRef();
	 	 	 	static AutoRef* getInstance(const AI::AIPackage &ai, const AI::HL::W::World &world);



				static  AI::AutoRef *instance;
				void multiple_defenders();
				void last_team_to_touch();
				void ball_has_left();
				void display();
				void violent_collision();
				void tick();



 	 private:
				explicit AutoRef(const AI::AIPackage &ai, const AI::HL::W::World &world);
				const AI::AIPackage &ai;
				const AI::HL::W::World &world;
				AI::Common::Team<AI::HL::W::Player, AI::BE::Player> FT;
				AI::Common::Team<AI::HL::W::Robot, AI::BE::Robot> ET;

				std::vector<std::deque<std::pair<Point, AI::Timestamp>>> FP;
				std::vector<std::deque<std::pair<Point, AI::Timestamp>>> EP;

				//this offset is when checking if a player has the ball. A player has the ball if the distance
				//from the center of the robot to the ball< robot.max_radius + offset
				//adjust this value to get a little more accuracy
				double offset = 0.004;

				//this will tell you if a ball is out of play
				bool ball_is_out_of_play;
				//this value is used so that the autoref only prints one message regarding this event
				bool print_ball_out_of_play;

				//this tells you the last team to touch the ball
				//can either be FRIENDLY_TEAM, ENEMY_TEAM, NO_TEAM
				Team last_touch;

				//These instance variables are set appropriately whenever triggered.
				bool enemy_in_defense_circle;
				bool friendly_in_defense_circle;
				bool partial_violation_e;
				bool full_violation_e;
				bool partial_violation_f;
				bool full_violation_f;
				bool print_partial_violation_f;
				bool print_full_violation_f;
				bool print_partial_violation_e;
				bool print_full_violation_e;

				//this is used at the beginning to throwaway some packets.
				//don't know if this helps against random data
				int throwaway;

				//set this field to false if you do not want to print any messages about ball velocity
				bool print_ball_velocity;

				//set when the velocity is greater than 8
				bool velocity_greater_than_eight;
				bool print_velocity_message;


				bool print_last_team_to_touch_ball;


				//set this to true in the constructor if you want more detail about certain processes.
				bool verbose;

				bool update_enemy_past;
				bool update_friendly_past;


				void on_vision_packet(AI::Timestamp ts, const SSL_WrapperPacket &vision_packet);
				void on_refbox_packet(AI::Timestamp ts, const SSL_Referee &packet);
				bool right_side_of_field(double x, Team t);
				Team analyze_the_past(unsigned int id_f, unsigned int id_e);


};



 /*static AutoRef& getInstance(const AI::AIPackage &ai, const AI::HL::W::World &world){
	 if(!(&AI::AutoRef::instance)){
		 AI::AutoRef::instance = new AutoRef(ai, world);
	 	}else
	 		return &AI::AutoRef::instance;
 }*/

}



#endif
