#include "ai/hl/stp/tactic/pass_gradient.h"
#include "ai/hl/stp/gradient_approach/optimizepass.h"
#include "ai/hl/stp/gradient_approach/ratepass.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/cshoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/gradient_approach/passMainLoop.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace GradientApproach = AI::HL::STP::GradientApproach;

using AI::HL::STP::Coordinate;

using AI::HL::STP::min_pass_dist;

namespace {

	BoolParam true_value_param(u8"Pass Gradient: use true t_delay and ball_vel values", u8"AI/STP/Tactic/gradient_pass", true);
	DoubleParam t_delay_param(u8"Pass Gradient: Time Delay", u8"AI/STP/Tactic/gradient_pass", 0.2, 0.0, 100.0);
	DoubleParam ball_vel_param(u8"Pass Gradient: Ball Velocity", u8"AI/STP/Tactic/gradient_pass", 4.0, 0.0, 100.0);
	DoubleParam x_param(u8"Pass Gradient: X-Location", u8"AI/STP/Tactic/gradient_pass", 0, -100.0, 100.0);
	DoubleParam y_param(u8"Pass Gradient: Y-Location", u8"AI/STP/Tactic/gradient_pass", 0, -100.0, 100.0);

	BoolParam toggle_main_loop_param(u8"Pass Gradient: Toggle main loop.", u8"AI/STP/Tactic/gradient_pass", true);

	Point target;
	Point target2;

	int counter;


	/*
	struct kick_info {
			Point target1;
			double shot_vel1;
			double t_delay1;

			Point target2;
			double shot_vel2;
			double t_delay2;

			bool kicked;
	};
*/
	struct PasserGradient : public Tactic {

		// HYSTERISIS

		double shot_velocity = 0;
		double time_delay;

		//keeps track of changes in toggle main loop
		bool mutable toggle_main_loop;


		PasserGradient(World world) : Tactic(world, true){
			GradientApproach::PassInfo::Instance().tacticInfo.kick_attempted = false;
			toggle_main_loop = toggle_main_loop_param;
			counter = 0;
			
		}

		bool done() const {
			return GradientApproach::PassInfo::Instance().tacticInfo.kick_attempted  && player.autokick_fired();
			
			//return player && kick_attempted && player.autokick_fired();
		}

		void player_changed() {


		}

		bool fail() const {

			/*if (!target) {
				return false;
			}
			*/
			/*if (!Evaluation::passee_suitable(world, target)) {
				return true;
			}
			*/

			return false;
		}

		Player select(const std::set<Player> &players) const {
			return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(world.ball().position()));
		}

		void execute() {
			if(counter % 16 == 0){

			
				std::vector<GradientApproach::PassInfo::passDataStruct> points = GradientApproach::PassInfo::Instance().getCurrentPoints();

				auto bestPass = std::max_element(points.begin(),
												points.end(),
												[] (GradientApproach::PassInfo::passDataStruct lhs,
														GradientApproach::PassInfo::passDataStruct rhs) {return lhs.quality < rhs.quality;});


				target = Point(bestPass->params.at(0),bestPass->params.at(1));
				shot_velocity = bestPass->params.at(3);
				//shot_velocity = 1;
				std::cout << "Ball_vel from Tact: " << shot_velocity << std::endl;
				if(shot_velocity < 2) shot_velocity = 2;
				time_delay = bestPass->params.at(2);

				GradientApproach::PassInfo::Instance().tacticInfo.kicker_location = player.position();
				GradientApproach::PassInfo::Instance().tacticInfo.kicker_orientation = player.orientation();
				GradientApproach::PassInfo::Instance().tacticInfo.kicker_target = target;

			}counter++;


			
			double passee_dist = (GradientApproach::PassInfo::Instance().tacticInfo.passee_location - target).len();
			double t_ball_arrive = (target - player.position()).len()/shot_velocity;
			double V_MAX = 2;
			double A_MAX = 3;
			double r;

			if (t_ball_arrive > 2*V_MAX/A_MAX){
			    r = V_MAX*V_MAX/A_MAX + (t_ball_arrive - 2*V_MAX/A_MAX)*V_MAX - passee_dist;
			}

			else{
			    r =  A_MAX/4*t_ball_arrive*t_ball_arrive - passee_dist;
			}

			if(r > 0 ){
				GradientApproach::PassInfo::Instance().tacticInfo.kick_attempted = TRUE;
				Action::cshoot_target(world, player,  target,  shot_velocity);
			}else{
				GradientApproach::PassInfo::Instance().tacticInfo.kick_attempted = FALSE;
				Action::cshoot_target(world, player,  target,  0);
			}
				
		

			

			/*
			Point relative_pos = world.ball().position()-player.position();

			if (relative_pos.len()<0.5) {
				std::cout << "shooting"
				//std::cout << "I had the ball not";
				Action::intercept_pivot(world, player, target, 0.05);

			
			}else if(false ){ //time_delay > 1){

				Action::cshoot_target(world, player,  target, 0);
				
			}else{
				Action::cshoot_target(world, player,  target,  shot_velocity);
			}
*/



			player.flags(0);

		}

		//Used to display gradient that is to be optimized
		void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
			//std::cout << "draw" << std::endl;


			//get paramaters


			double time_delay_draw;
			double shot_velocity_draw;

			time_delay_draw = t_delay_param;
			shot_velocity_draw = ball_vel_param;


/*
			ctx->set_line_width(0.025);
			ctx->set_source_rgba(100, 100, 100, 0.9);
			//draw a circle at the target location
			ctx->arc(target.x, target.y, 0.05, 0.0, 2*M_PI);
			ctx->fill();
	*/

			//increment through rectangle

			/*for(int x = -40; x<=40; x++){
				for(int y = -20; y<=20; y++){
					//find physical locations
					double yLocation = y*0.1;
					double xLocation = x*0.1;
					//check if out of bounds
					GradientApproach::PassInfo::Instance().updateWorldSnapshot(world);
					if(yLocation > -world.field().width()/2-0.4 && yLocation < world.field().width()/2+0.4
							&& xLocation > -world.field().length()/2-0.4 && xLocation < world.field().length()/2+0.4){
						//calculate quality

						//std::cout << "start get snapshot" << std::endl;
						double quality = GradientApproach::ratePass(GradientApproach::PassInfo::Instance().getWorldSnapshot(),Point(xLocation,yLocation),time_delay_draw,shot_velocity_draw);
						
						//std::cout << "finish get snapshot. Quality: " << quality << std::endl;


						//color scaled by quality
						ctx->set_source_rgba(quality*255, quality*255,quality*255 , 0.5);


						//draw a circle
						ctx->arc(xLocation, yLocation, 0.04, 0.0, 2*M_PI);
						ctx->fill();
						

						if(yLocation){
							ctx->set_font_size(0.15);
							ctx->move_to(xLocation, yLocation);
							ctx->set_source_rgba(0,0,0,1);
							//ctx->show_text(std::to_string(quality));
						}

					}
				}
			}*/
			//std::cout << "finished loop" << std::endl;
/*
			ctx->set_line_width(0.025);
			ctx->set_source_rgba(100, 100, 100, 0.9);
			//Draw a circle at the target location
			ctx->arc(target.x, target.y, 0.05, 0.0, 2*M_PI);
			ctx->fill();
*/
			//Draw player(Aqua)
			//ctx->set_source_rgba(0, 0.5, 1, 0.9);
			//ctx->arc(player.position().x, player.position().y, 0.05, 0.0, 2*M_PI);
			//ctx->fill();

			/*
			//Draw Passee(black)
			ctx->set_source_rgba(0, 0, 0, 0.9);
			ctx->arc(passee.position().x, passee.position().y, 0.05, 0.0, 2*M_PI);
			ctx->fill();


			//Print out the quality at point 1,1(sanity check)
			ctx->set_source_rgba(0, 0, 0, 1);
			ctx->set_font_size(0.15);
			ctx->move_to(0, -world.field().width()/2);
			ctx->set_source_rgba(0,0,0,1);

			double quality = GradientApproach::ratePass(world,Point(x_param, y_param),time_delay_draw,shot_velocity_draw,passer);
			ctx->show_text(std::to_string(quality));
*/




			if(toggle_main_loop_param != toggle_main_loop){
				toggle_main_loop = toggle_main_loop_param;
				GradientApproach::testLoop(GradientApproach::PassInfo::Instance().getWorldSnapshot());
			}

			//std::cout << "toggled" << std::endl;

			std::vector<GradientApproach::PassInfo::passDataStruct> points = GradientApproach::PassInfo::Instance().getCurrentPoints();
			double colour_intensity;
			//std::cout << "got points" << std::endl;

			for(unsigned int i = 0; i < points.size(); i++){
				colour_intensity = std::pow(points.at(i).quality, 0.5);
				ctx->set_source_rgba(colour_intensity,colour_intensity,colour_intensity, 1);
		
				ctx->arc(points.at(i).params.at(0), points.at(i).params.at(1), 0.04, 0.0, 2*M_PI);
				ctx->fill();
			}

			Point dest = Evaluation::passee_position();

			ctx->set_source_rgba(1,0.2,0.2,1);
			ctx->arc(dest.x, dest.y, 0.04, 0.0, 2*M_PI);
			ctx->fill();

			ctx->set_source_rgba(0,0,1,1);
			ctx->arc(target.x, target.y, 0.08, 0.0, 2*M_PI);
			ctx->fill();

			//std::cout << "drew points" << std::endl;

			/*auto bestPass = std::max_element(points.begin(),
								points.end(),
								[] (GradientApproach::PassInfo::passDataStruct lhs,
										GradientApproach::PassInfo::passDataStruct rhs) {return lhs.quality < rhs.quality;});


			//Point bestTarget = Point(bestPass->params.at(0),bestPass->params.at(1));

			ctx->set_source_rgba(255, 255,255 , 1);
			//ctx->arc(bestPass->params.at(0), bestPass->params.at(1), 0.08, 0.0, 2*M_PI);
			ctx->fill();



			std::cout << "finish draw" << std::endl;
			*/
		}




		
		Glib::ustring description() const {
			return u8"passer-gradient";
		}
		
		
	};

	struct PasseeGradient : public Tactic {


		PasseeGradient(World world) : Tactic(world, false) {
		}

		Player select(const std::set<Player> &players) const {
			std::vector<GradientApproach::PassInfo::passDataStruct> points = GradientApproach::PassInfo::Instance().getCurrentPoints();
			auto bestPass = std::max_element(points.begin(),points.end(),[] (GradientApproach::PassInfo::passDataStruct lhs, GradientApproach::PassInfo::passDataStruct rhs) {return lhs.quality < rhs.quality;});

			if(points.size() > 1){
					target = bestPass->getTarget();
			}else{
					target = player.position();
			}
			
			Player passee = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(target));
			GradientApproach::PassInfo::Instance().tacticInfo.passee_location = passee.position();
			return passee;
		}

	

		void execute() {

			//Point target = GradientApproach::PassInfo::Instance().get_target();
			GradientApproach::PassInfo::worldSnapshot snapshot =  GradientApproach::PassInfo::Instance().getWorldSnapshot();
			if(counter % 16 == 0){
				if(world.ball().velocity().len() > 1.5){
					Action::intercept(player, snapshot.passer_position);
				}else{

					Action::move( player, (GradientApproach::PassInfo::Instance().tacticInfo.kicker_location- target).orientation(), target);
				}
			}

		}


		Glib::ustring description() const {
			return u8"passee-gradient";
		}
	};




	struct PasseeGradientSecondary : public Tactic {


			PasseeGradientSecondary(World world) : Tactic(world, false) {
			}

			Player select(const std::set<Player> &players) const {
				std::vector<GradientApproach::PassInfo::passDataStruct> points = GradientApproach::PassInfo::Instance().getCurrentPoints();
				if(points.size() > 1){
					target2 = points.at(1).getTarget();
				}else{
					target2 = player.position();
				}
				
				Player passee = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(target2));

				return passee;
			}

		

			void execute() {
				//Point target = GradientApproach::PassInfo::Instance().get_target();

				Action::move( player, (GradientApproach::PassInfo::Instance().tacticInfo.kicker_location- target2).orientation(), target2);

			}

			Glib::ustring description() const {
				return u8"passee-gradient-secondary";
			}
		};


	struct PasseeReceive : public Tactic{

		PasseeReceive(World world) : Tactic(world, true){

			GradientApproach::PassInfo::kick_info passer_info = GradientApproach::PassInfo::Instance().tacticInfo;

			passer_info.alt_passer_orientation = (passer_info.kicker_location-passer_info.kicker_target).orientation();
			passer_info.alt_passer_point = passer_info.kicker_location;

			passer_info.use_stored_point_as_passer = true;

		}

		~PasseeReceive(){

			GradientApproach::PassInfo::Instance().tacticInfo.use_stored_point_as_passer = false;
		}

		Player select(const std::set<Player> &players) const {
			Point target = GradientApproach::PassInfo::Instance().tacticInfo.kicker_target;
			Player passee = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(target));

			return passee;
		}

		bool done() const {
			return player && player.has_ball();
		}
		bool fail() const {/*
			GradientApproach::PassInfo::kick_info passer_info = GradientApproach::PassInfo::Instance().tacticInfo;
			Point intercept_pos = closest_lineseg_point(passer_info.kicker_target, world.ball().position(), world.ball().position(100));
			if((passer_info.kicker_target - world.ball().position()).len() > 0.5 && world.ball().velocity().len() < 0.1){
				return true;
			}
			
			double possible_dist;
			double V_MAX = 2;
			double A_MAX = 3;
			double t_ball_arrive = (world.ball().position() - intercept_pos).len()/world.ball().velocity().len();
			//can we get there in time?
			if (t_ball_arrive > 2*V_MAX/A_MAX){
			    possible_dist = V_MAX*V_MAX/A_MAX + (t_ball_arrive - 2*V_MAX/A_MAX)*V_MAX;
			}

			else{
			    possible_dist =  A_MAX/4*t_ball_arrive*t_ball_arrive;
			}


			if( (player.position() - intercept_pos).len() - possible_dist > -0.5){
				return false;
			}else{
				return true;
			}
			*/
			return false;

		}

		void execute(){
			std::cout << "Running passee receive" << std::endl;

			GradientApproach::PassInfo::kick_info passer_info = GradientApproach::PassInfo::Instance().tacticInfo;
			Point intercept_pos = closest_lineseg_point(passer_info.kicker_target, world.ball().position(), world.ball().position(100));
			Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos);
			player.type(AI::Flags::MoveType::DRIBBLE);
		}

		Glib::ustring description() const override {
			return u8"passee_gradient-receive";
		}
	};

}

Tactic::Ptr AI::HL::STP::Tactic::passer_gradient(World world) {
	Tactic::Ptr p(new PasserGradient(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_gradient(World world) {
	Tactic::Ptr p(new PasseeGradient(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_gradient_secondary(World world) {
	Tactic::Ptr p(new PasseeGradientSecondary(world));
	return p;
}



Tactic::Ptr AI::HL::STP::Tactic::passee_gradient_receive(World world) {
	Tactic::Ptr p(new PasseeReceive(world));
	return p;
}
