
#include "ai/hl/stp/tactic/pass_simple.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/enemy_risk.h"
#include "ai/hl/stp/evaluation/friendly_capability.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/dprint.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/gradient_approach/ratepass.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <assert.h>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Evaluation;
using namespace AI::HL::W;
using namespace AI::HL::STP::GradientApproach;
namespace Action = AI::HL::STP::Action;

using AI::HL::STP::Coordinate;

using AI::HL::STP::min_pass_dist;

namespace {
    Angle getOnetimeShotDirection(World world, Player player, Point pos){
        Point shot = get_best_shot(world, player);
        Angle shotDir = (shot - pos).orientation();
        Point shotVector = Point::of_angle(shotDir);

        Point botVector = shotVector.norm();

        Point ballVel = world.ball().velocity();
        Point lateralVel = ballVel - (ballVel.dot(-botVector))*(-botVector); 
        double lateralSpeed = 0.3*lateralVel.len();
        double kickSpeed = 5.5;
        Angle shotOffset = Angle::asin(lateralSpeed/kickSpeed);
        
        //check which direction the ball is going in so we can decide which direction to apply the offset in
        if(lateralVel.dot(shotVector.rotate(Angle::quarter())) > 0){
            // need to go clockwise
            shotOffset = - shotOffset;
        }
        return shotDir + shotOffset;
    }
	Point passer_pos;
	Point targets[3];
	Point kicktarget;
    double kickspeed;
	Point intercept_pos;
	Point passee_pos;
	Angle passee_angle;
	bool ball_kicked = false;
    int ticks_since_kick =0;

	class PasserSimple final : public Tactic {
		public:
			explicit PasserSimple(World world, bool shoot_on_net) : Tactic(world), kick_attempted(false), shoot_if_possible(shoot_on_net) {
                ball_kicked = false;
                ticks_since_kick = 0;
			}
		
		private:
			bool shoot_if_possible;
			bool kick_attempted;
			bool done() const {
				if(kick_attempted  && player().autokick_fired()){
                    ball_kicked = true;
					return true;
				}
				return false;
			}

			void player_changed() {}

			bool fail() const {
				//TODO: add failure condition 
				//TODO: add this back in (look at svn history)
				return false;
			}

			Player select(const std::set<Player> &players) const {
				Player passer;

				for(auto it = players.begin(); it != players.end(); it++)
				{
					if(it->get_impl()->pattern() == FAVOURITE_BOT)
					{
						passer = *it;
					}
				}

				if(!passer)
				{
					passer =  *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(world.ball().position()));
				}

				passer_pos = passer.position();
				if(world.ball().velocity().len() < 0.3){
					PassInfo::Instance().setAltPasser(world.ball().position(), Angle::zero());
				}else{
					PassInfo::Instance().setAltPasser(passer.position(), passer.orientation());
				}
				return passer;
			}

			void execute(caller_t& ca) {
				PassInfo::passDataStruct pass = PassInfo::Instance().getBestPass();
				PassInfo::passDataStruct testPass;

				targets[0] = Point(pass.params.at(0), pass.params.at(1));

                PassInfo::worldSnapshot snapshot =  PassInfo::Instance().convertToWorldSnapshot(world);
				double currentPassScore = ratePass(snapshot,Point(pass.params.at(0), pass.params.at(1)), pass.params.at(2), pass.params.at(3));
				double testPassScore;


				double time_since_start = 0.0;
				while(time_since_start < 10.0){
					time_since_start += 0.03;

					testPass = PassInfo::Instance().getBestPass();
					
                	PassInfo::worldSnapshot snapshot =  PassInfo::Instance().convertToWorldSnapshot(world);
                    currentPassScore = ratePass(snapshot,Point(pass.params.at(0), pass.params.at(1)), pass.params.at(2), pass.params.at(3));
                    testPassScore = ratePass(snapshot,Point(testPass.params.at(0), testPass.params.at(1)), testPass.params.at(2), testPass.params.at(3));

                    printf(", pass score: %f",currentPassScore);
					if(testPassScore > 1.8*currentPassScore || (testPassScore > 0.08 && currentPassScore < 0.08)){
						pass = testPass;
						targets[0] = Point(pass.params.at(0), pass.params.at(1));
						currentPassScore = testPassScore;
                        printf("SWITCHING SPOTSSS!");
					}


                    if(testPassScore > 0.08 && (passee_pos-targets[0]).len() < 1.3*Robot::MAX_RADIUS){//(pass.params.at(2) < 0.6 ||
                        kick_attempted = true;
                        passer_pos = world.ball().position();
                        kicktarget = passee_pos; //targets[0];
                        kickspeed = 4.0;//pass.params.at(3);

                        Action::shoot_target(ca, world, player(),  targets[0], kickspeed);
                    }else{
                        printf("delay time too large: %f",pass.params.at(2));
                        Action::get_behind_ball(ca, world, player(), targets[0]);
//				    	player().flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_TINY);
//                        Action::move(ca, world, player(),  world.ball().position() - (targets[0] - world.ball().position()).norm(0.2), (targets[0] - world.ball().position()).orientation());
                    }
                    yield(ca);
				}
				// Just chip it up the field	
				targets[0] = Point(3.1, 0.0);

				while(true){
                	double shootScore = get_best_shot_pair(world, player()).second.to_degrees();
                    
                    if(shoot_if_possible && shootScore > 2.0){
						Action::shoot_goal(ca, world, player());
                    }else{
                        kick_attempted = true;
						Action::shoot_target(ca, world, player(), targets[0], (world.ball().position() - targets[0]).len()/2.0, true); 
                        passer_pos = world.ball().position();
                        kicktarget = targets[0];
                        kickspeed = 2.0;
					}
                    yield(ca);
                }
			}
			
			Glib::ustring description() const {
				return u8"passer-simple";
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
				ctx->set_source_rgba(1.0, 1.0, 1.0, 4.0);
				ctx->arc(targets[0].x, targets[0].y, 0.08, 0.0, 2*M_PI);
				ctx->fill();
			}
	};

	class PasseeSimple final : public Tactic {

		public:
			PasseeSimple(World world, unsigned index) : Tactic(world), index(index) {
			}
		private:
			unsigned index;
			Player select(const std::set<Player> &players) const {
				Player passee; 
				if(index > 0 || ball_kicked == true){
					std::vector<PassInfo::passDataStruct> bestPasses = PassInfo::Instance().getCurrentPoints();
					if(bestPasses.size() > index && bestPasses.at(index).quality > 0.01){
						targets[index] = Point(bestPasses.at(index).params.at(0), bestPasses.at(index).params.at(1));
						std::cout << "target "<< index << ": " << targets[index].x << ", " << targets[index].y;
						std::cout << "quality "<< bestPasses.at(index).quality;
					}else{
						targets[index] = world.ball().position() + 1.5*index*(world.field().friendly_goal() - world.ball().position()).norm();
					}
				}

				std::set<Player> good_passees;
				for (Player player : players)
				{
					for (unsigned int i = 0; i < world.working_dribblers.size(); i++)
					{
						if(player.get_impl()->pattern() == world.working_dribblers[i])
                        {
                            good_passees.insert(player);
                            std::cout << "player " << player.get_impl()->pattern()
                                      << " has good dribbler" << std::endl;
                        }
					}
				}

				if(good_passees.size() > 0)
				{
					passee = *std::min_element(good_passees.begin(), good_passees.end(),
											   AI::HL::Util::CmpDist<Player>(targets[index]));
					std::cout << good_passees.size() << " good passees" << std::endl;
				}
				else
				{
					passee = *std::min_element(players.begin(), players.end(),
											   AI::HL::Util::CmpDist<Player>(targets[index]));
				}

                if(index==0){
                    passee_pos = passee.position();
                    passee_angle = passee.orientation();
                }
				return passee;
			}

		

			void execute(caller_t& ca) {
				while(true){
                    PassInfo::worldSnapshot snapshot =  PassInfo::Instance().convertToWorldSnapshot(world);
                    //double score = get_passee_shoot_score(snapshot, targets[index]);
				    //std::cout << "shoot score "<< score;

                	Angle botDir = get_best_shot_pair(world, player()).second;
                    if(botDir.angle_diff((world.ball().position() - targets[index]).orientation()).to_degrees() >65.0){
                        botDir = (world.ball().position() - targets[index]).orientation();
                    }
                    printf("trying to aim in %f direction", botDir.to_degrees());
                    Point offset = -Point::of_angle(botDir)*Robot::MAX_RADIUS;

                    Action::move(ca, world, player(), targets[index] + offset, botDir); // autokick off

					yield(ca);
				}

			}


			Glib::ustring description() const {
				return u8"passee-simple";
			}
	};




	class PasseeReceive final : public Tactic{
		public:
			PasseeReceive(World world) : Tactic(world){
			}
		private:
            double timeSinceKick;
			~PasseeReceive(){
			}

			Player select(const std::set<Player> &players) const {
				Player passee = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(kicktarget));
				return passee;
			}

			bool done() const {
//				if(player() && (player().has_ball() || player().autokick_fired())) return true;
				std::cout << "ball speed: "<< world.ball().velocity().len();
				if(world.ball().velocity().len() < 0.1 && timeSinceKick > 0.5){
					return true;
				}
                //todo: put back in fail section


                Point expectedPos = passer_pos + (kicktarget - passer_pos).norm()*kickspeed*timeSinceKick;

				if((world.ball().position() - expectedPos).len() > 4.0){
					return true;
				}


				return false;
			}
			bool fail() const {
				return false;
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
				/*ctx->set_source_rgba(1.0, 1.0, 1.0, 4.0);
				ctx->arc(targets[0].x, targets[0].y, 0.08, 0.0, 2*M_PI);
				ctx->fill();
                */
				ctx->set_source_rgba(1.0, 0.0, 1.0, 4.0);
				ctx->arc(targets[1].x, targets[1].y, 0.08, 0.0, 2*M_PI);
				ctx->fill();
				ctx->set_source_rgba(1.0, 1.0, 0.0, 4.0);
				ctx->arc(targets[2].x, targets[2].y, 0.08, 0.0, 2*M_PI);
				ctx->fill();

				ctx->set_source_rgba(0.0, 0.0, 0.0, 4.0);
				ctx->arc(intercept_pos.x, intercept_pos.y, 0.02, 0.0, 2*M_PI);
				ctx->fill();
			}

			void execute(caller_t& ca){
                double shootScore = get_best_shot_pair(world, player()).second.to_degrees();
				bool can_shoot = true;//(shootScore > 0.1);
                timeSinceKick = 0.03;
				while(true){
                    timeSinceKick += 0.03;
					//TODO: add logic for one time shot on net
					//TODO: consider moving interception with ball 
                    //maybe could also use the direction kicker is pointing when it is kicked?
    				intercept_pos = closest_lineseg_point(kicktarget, world.ball().position(), world.ball().position(100));
					PassInfo::Instance().setAltPasser(intercept_pos, world.ball().velocity().rotate(Angle::half()).orientation());

                    //todo: uncomment
					PassInfo::passDataStruct pass = PassInfo::Instance().getBestPass();
					targets[0] = Point(pass.params.at(0), pass.params.at(1));

                    Angle botDir = getOnetimeShotDirection(world, player(), intercept_pos);
                    
                    if(can_shoot && botDir.angle_diff((passer_pos - intercept_pos).orientation()).to_degrees() < 60.0){
                        //Point alteredPos = player().position() + 1.0*(intercept_pos - player().position());
                        Point offset = -Point::of_angle(botDir)*Robot::MAX_RADIUS;

                        Action::move(ca, world, player(), intercept_pos + offset, botDir, true, false ); // autokick on
                    }else{
                        //TODO: turn dribbler on
                        Action::move(ca, world, player(), intercept_pos, (passer_pos - intercept_pos).orientation(), false, true); // autokick off
                    }

					yield(ca);
				}
			}

			Glib::ustring description() const override {
				return u8"passee_simple-receive";
			}
	};

}

Tactic::Ptr AI::HL::STP::Tactic::passer_simple(World world, bool shoot_on_net) {
	Tactic::Ptr p(new PasserSimple(world, shoot_on_net));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_simple(World world, unsigned index) {
	Tactic::Ptr p(new PasseeSimple(world, index));
	return p;
}



Tactic::Ptr AI::HL::STP::Tactic::passee_simple_receive(World world) {
	Tactic::Ptr p(new PasseeReceive(world));
	return p;
}
