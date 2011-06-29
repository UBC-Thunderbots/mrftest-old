#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play_executor.h"
#include "geom/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/team.h"
#include "util/dprint.h"
#include "ai/hl/stp/predicates.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Coordinate;

namespace {

	DoubleParam passer_tol_target("when passer within this angle tol passee responds to passer direction", "STP/Tactic/pass", 30.0, 0.0, 180.0);
	DoubleParam fast_velocity("velocity of pass threshold", "STP/Tactic/pass", 1.0, 0.0, 1.0);
	DoubleParam negligible_velocity("velocity to ignore", "STP/Tactic/pass", 0.05, 0.0, 1.0);
	DoubleParam passee_hack_dist("Hack to get reciever to move more quickly to intercept pos by modifying dest (meters)", "STP/Tactic/pass", 0.0, 0.0, 1.0);
	DoubleParam ball_region_param(" the radius (meters) in which passer must be with repect to ball before valid ", "STP/Tactic/pass", 1.0, 0.0, 5.0);
	DoubleParam target_region_param(" the radius (meters) in which passee must be with repect to target before valid ", "STP/Tactic/pass", 2.0, 0.0, 5.0);
//	double passer_tol_target = 30.0; 
//	double negligible_velocity = 0.1;
//	double passee_hack_dist = 0.0;
	
	struct kick_info {
		Point kicker_location;
		double kicker_orientation;
		Point kicker_target;
		bool kicked;
	};

	class PasserShoot : public Tactic {
		public:
			PasserShoot(const World &world) : Tactic(world, true), dynamic(true), kicked(false) {
				Point dest = Evaluation::passee_position(world);
				loc = dest;		
			}

			PasserShoot(const World &world, Coordinate target) : Tactic(world, true), dynamic(false), target(target), kicked(false) {
			}
			static kick_info passer_info;
			
		private:
			bool dynamic;
			bool kicked;
			Coordinate target;
			Point loc;

			bool done() const {
#warning TODO allow more time
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();				
				//return kicked && (player->position() - world.ball().position()).len() > (player->position() - dest).len()/4;
				return kicked || player->autokick_fired();
			}
			
			bool fail() const {
				return player.is() && AI::HL::Util::calc_best_shot_target(world, target.position(), player, 0.5, true).second == 0;
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players);
			}

			void execute() {
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();
				kicked = kicked || player->autokick_fired();
				if (!player->autokick_fired() && !kicked) {
					PasserShoot::passer_info.kicker_location = player->position();
					PasserShoot::passer_info.kicker_orientation = player->orientation();
					PasserShoot::passer_info.kicker_target = dest;
					PasserShoot::passer_info.kicked = false;
				} else {
					PasserShoot::passer_info.kicked = true;
				}
				//Player::CPtr passee = Evaluation::nearest_friendly(world, dest);
				Action::shoot_pass(world, player, dest);
			}

			std::string description() const {
#warning TODO give more information
				return "passer-shoot";
			}
	};
	
	kick_info PasserShoot::passer_info;	
	Player::Ptr last_passee;
	
	void on_robot_removing(std::size_t i, const World &w) {
		if(w.friendly_team().get(i) == Player::CPtr(last_passee)){
			last_passee.reset();
		}
	}
	
	void connect_remove_player_handler(const World &w){
		static bool connected = false;	
		if(!connected){
			w.friendly_team().signal_robot_removing().connect(sigc::bind(&on_robot_removing, sigc::ref(w)));
			connected = true;
		}
	}

	class PasseeMove : public Tactic {
		public:
			PasseeMove(const World &world) : Tactic(world, false), dynamic(true), target(target) {
				connect_remove_player_handler(world);
			}

			PasseeMove(const World &world, Coordinate target) : Tactic(world, false), dynamic(false), target(target) {
			}

		private:
			bool dynamic;
			Coordinate target;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();
				last_passee = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
				return last_passee;
			}

			void execute() {
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();
				Player::CPtr passer = Evaluation::nearest_friendly(world, world.ball().position());
				kick_info passer_info = PasserShoot::passer_info;
				bool fast_ball = world.ball().velocity().len() > fast_velocity;
				#warning looks abit ugly.. 
				#warning as if the shit written above isn't butt ugly
				// to adjust for passer orientation make sure passer is within a radius of ball, the ball is in front of robot
				// and robot is within some angle threshold of the target
				// the other condition is if the ball was just kicked, use the information about passer when ball was just kicked ( and passer information is more reliable than ball velocity information)
				bool match_passer_ori = (Action::within_angle_thresh(passer_info.kicker_location, passer_info.kicker_orientation, passer_info.kicker_target, passer_tol_target) && !passer_info.kicked);
				match_passer_ori = match_passer_ori && (passer_info.kicker_location - world.ball().position()).len() < ball_region_param;
				match_passer_ori = match_passer_ori && (world.ball().position() - passer_info.kicker_location).dot(passer_info.kicker_target - passer_info.kicker_location) > 0;
				match_passer_ori = match_passer_ori || (passer_info.kicked && !fast_ball);
				
				if ( match_passer_ori ) {
					Point pass_dir(100, 0);
					pass_dir = pass_dir.rotate(passer_info.kicker_orientation);
					Point intercept_pos = closest_lineseg_point(player->position(), passer_info.kicker_location, passer_info.kicker_location + pass_dir);
					Point addit = passee_hack_dist*(intercept_pos - player->position()).norm();
					Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos + addit);
				} else if (passer_info.kicked && fast_ball) {
					Point intercept_pos = closest_lineseg_point(player->position(), world.ball().position(),  world.ball().position() + 100 * (world.ball().velocity().norm()));
					Point pass_dir = (world.ball().position() - passer_info.kicker_location).norm();
					Point addit = passee_hack_dist*(intercept_pos - player->position()).norm();
					Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos + addit);
				} else {
					Action::move(player, (world.ball().position() - player->position()).orientation(), passer_info.kicker_target);
				}
				player->type(AI::Flags::MoveType::DRIBBLE);
			}

			std::string description() const {
				return "passee-move";
			}
	};
	

	
	class PasseeRecieve : public Tactic {
		public:
			PasseeRecieve(const World &world) : Tactic(world, true) {
				#warning find a good mechanism for passing 
			}

		private:
			
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				// hard to calculate who is best to recieve the pass
				// so use whoever last was assigned if they are still around
				// closeness to "intended target" is a terrible way to choose
				// so only do so if absolutely necessary
				if (last_passee.is() && players.find(last_passee) != players.end()) {
					return last_passee;
				}
				//otherwise we don't really have a choice but to use the one closest to the "intended target"
				const Point dest = PasserShoot::passer_info.kicker_location;
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
			}
			bool done() const {
				return player.is() && Evaluation::possess_ball(world, player);
			}
			void execute() {
				
				const kick_info &passer_info = PasserShoot::passer_info;
				
				bool fast_ball = world.ball().velocity().len() > fast_velocity;				
				
				// ball heading towards us
				bool can_intercept = ((player->position() - world.ball().position()).dot(world.ball().velocity()) > 0);
				
				if(world.ball().velocity().len() < negligible_velocity) {
					Action::chase(world, player);
					player->type(AI::Flags::MoveType::DRIBBLE);
					return;
				}
				
				if (!fast_ball) {
					Point pass_dir(100, 0);
					pass_dir = pass_dir.rotate(passer_info.kicker_orientation);
					Point intercept_pos = closest_lineseg_point(player->position(), passer_info.kicker_location, passer_info.kicker_location + pass_dir);
					Point addit = passee_hack_dist*(intercept_pos - player->position()).norm();
					Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos + addit);
				} else if (can_intercept && fast_ball) {
					
					Point intercept_pos = closest_lineseg_point(player->position(), world.ball().position(),  world.ball().position() + 100 * (world.ball().velocity().norm()));
					Point pass_dir = (world.ball().position() - passer_info.kicker_location).norm();
					Point addit = passee_hack_dist*(intercept_pos - player->position()).norm();
					Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos + addit);
				} else {
					// ball is running too slowly, chase it
					Action::chase(world, player);
				}
				player->type(AI::Flags::MoveType::DRIBBLE);
			}

			std::string description() const {
				return "passee-recieve";
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_shoot_target(const World &world, Coordinate target) {
	const Tactic::Ptr p(new PasserShoot(world, target));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_move_target(const World &world, Coordinate target) {
	const Tactic::Ptr p(new PasseeMove(world, target));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_receive_target(const World &world, Coordinate target) {
	const Tactic::Ptr p(new PasseeRecieve(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passer_shoot_dynamic(const World &world) {
	const Tactic::Ptr p(new PasserShoot(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_move_dynamic(const World &world) {
	const Tactic::Ptr p(new PasseeMove(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_receive(const World &world) {
	const Tactic::Ptr p(new PasseeRecieve(world));
	return p;
}


