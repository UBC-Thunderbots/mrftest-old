#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/evaluation/pass.h"
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
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;
namespace Predicates = AI::HL::STP::Predicates;

namespace {

	DoubleParam negligible_velocity("velocity to ignore", "STP/Tactic/pass", 0.1, 0.0, 1.0);
	DoubleParam passer_tol_target(" angle tolerance that the passer needs to be with respect to the target", "STP/Tatic/pass", 30.0, 0.0, 180.0);
	DoubleParam passer_tol_reciever(" angle tolerance that the passer needs to be with respect to the passee", "STP/Tatic/pass", 20.0, 0.0, 180.0);
	DoubleParam passee_tol(" distance tolerance that the passee needs to be with respect to the passer shot", "STP/Tatic/pass", 0.05, 0.0, 1.0);
	
	DoubleParam passee_hack_dist("Hack to get reciever to move more quickly to intercept pos by modifying dest (meters)", "STP/Tatic/pass", 0.03, 0.0, 1.0);

	class PasserShoot : public Tactic {
		public:
			PasserShoot(const World &world) : Tactic(world, true), kicked(false) {
			}

		private:
			bool kicked;
			bool done() const {
				Player::CPtr passee = Evaluation::nearest_friendly(world, Evaluation::calc_pass_positions(world).second);
				return kicked && (passee->has_ball() || Predicates::their_ball(world));
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players);
			}
			void execute() {
				kicked = false;
				std::pair<Point, Point> pp = Evaluation::calc_pass_positions(world);

				// dribble
				player->move(pp.first, (pp.second - player->position()).orientation(), Point());
				player->type(AI::Flags::MoveType::DRIBBLE);
				player->prio(AI::Flags::MovePrio::HIGH);
				kicked = Action::shoot_pass(world, player, pp.second);
			}
			std::string description() const {
				return "passer-shoot";
			}
	};
	
	class PasserShootTarget : public Tactic, public sigc::trackable {
		public:
			PasserShootTarget(const World &world, Point target) : Tactic(world, true), kicked(false), target(target) {
					world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &PasserShootTarget::on_player_removed));
			}
		private:
			bool kicked;
			Point target;
			mutable Player::Ptr passer;

			void on_player_removed(std::size_t index) {
				if( world.friendly_team().get(index) == Player::CPtr(passer)){
					passer.reset();
				}
			}

			bool done() const {
				return kicked && world.ball().velocity().len() > negligible_velocity;;
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				if(passer.is()){
					return passer;
				}
				passer = select_baller(world, players);
				return passer;
			}

			void execute() {
					if (Action::shoot_pass(world, player, target)) {
						kicked = true;
						AI::HL::STP::HACK::last_kicked = player;
					}
			}

			std::string description() const {
				return "passer-shoot-target";
			}
	};

	class PasseeMoveTarget : public Tactic, public sigc::trackable {
		public:
			PasseeMoveTarget(const World &world, Point target, bool active) : Tactic(world, active), target(target), valid(true) {
				world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &PasseeMoveTarget::on_player_removing));
			}
			
		virtual bool done() const {
			return player->has_ball() || !valid;
		}
		private:
			mutable Player::Ptr passee;
			Point target;
			bool valid;

			void on_player_removing(std::size_t index) {
				if( world.friendly_team().get(index) == Player::CPtr(passee)){
					passee.reset();
				}
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				if(passee.is() && players.find(passee) != players.end() ){
					return passee;
				}
				passee = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(target)) ;
				return passee;
			}

			void execute() {
				// Action specific to this tactic
				Player::Ptr passer;

				if(passee != AI::HL::STP::HACK::active_player){
					passer =  AI::HL::STP::HACK::active_player;
				}

				bool fast_ball = world.ball().velocity().len() > negligible_velocity;
				if(!passer.is()){ //no active passer
					if(fast_ball) {

						// if ball is moving away from robot not closer then we chase
						// if the ball is moving towards us then we want to intercept
						bool	need_chase = ( world.ball().position() - player->position() ).dot(world.ball().velocity()) > 0;

						if(need_chase){
							Action::chase(world, passee);
							valid = false;//this should be a fail condition
						}else{
							Point intercept_pos = closest_lineseg_point(player->position(), world.ball().position(),  world.ball().position() + 100 * world.ball().velocity().norm());
							Point addit = passee_hack_dist*(intercept_pos - passee->position()).norm();
							Action::move(player, (-world.ball().velocity()).orientation(), intercept_pos + addit);
						}

					} else {
						Action::chase(world, passee);
						valid = false;
					}
				} else { //there is an active passer
					if(Action::within_angle_thresh(passer, target, passer_tol_target) ){
						Point pass_dir(100, 0);
						pass_dir = pass_dir.rotate(passer->orientation());
						Point intercept_pos = closest_lineseg_point(player->position(), passer->position(), passer->position() + pass_dir);
						Point addit = passee_hack_dist*(intercept_pos - passer->position()).norm();
						Action::move(player, (passer->position() - intercept_pos).orientation(), intercept_pos + addit);
					}else{
						Action::move(player, (world.ball().position() - player->position()).orientation(), target);
					}
				}
			}

			std::string description() const {
				return "passee-target";
			}
	};
	
	class PasseeMove : public Tactic {
		public:
			PasseeMove(const World &world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				Point dest = Evaluation::calc_pass_positions(world).second;

				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
			}
			void execute() {
				Point dest = Evaluation::calc_pass_positions(world).second;
				Action::move(player, (world.ball().position() - player->position()).orientation(), dest);
				
				Player::CPtr passer = Evaluation::nearest_friendly(world, world.ball().position());
				
				bool fast_ball = world.ball().velocity().len() > negligible_velocity;
				if(Action::within_angle_thresh(passer, dest, passer_tol_target) && passer->has_ball()){
					Point pass_dir(100, 0);
					pass_dir = pass_dir.rotate(passer->orientation());
					Point intercept_pos = closest_lineseg_point(player->position(), passer->position(), passer->position() + pass_dir);
					Point addit = passee_hack_dist*(intercept_pos - passer->position()).norm();
					Action::move(player, (passer->position() - intercept_pos).orientation(), intercept_pos + addit);
				
				} else if (!passer->has_ball()) { 
					if(fast_ball) {

						// if ball is moving away from robot not closer then we chase
						// if the ball is moving towards us then we want to intercept
						bool need_chase = (world.ball().position() - player->position()).dot(world.ball().velocity()) > 0;

						if(need_chase){
							Action::chase(world, player);
						}else{
							Point intercept_pos = closest_lineseg_point(player->position(), world.ball().position(), world.ball().position() + 100 * world.ball().velocity().norm());
							Point add = passee_hack_dist*(intercept_pos - player->position()).norm();
							Action::move(player, (-world.ball().velocity()).orientation(), intercept_pos + add);
						}

					} else {
						Action::chase(world, player);
					}

				} else {
					Action::move(player, (world.ball().position() - player->position()).orientation(), dest);
				}				
				player->type(AI::Flags::MoveType::DRIBBLE);
			}
			std::string description() const {
				return "passee-move";
			}
	};

	class DefPasserShoot : public Tactic {
		public:
			DefPasserShoot(const World &world) : Tactic(world, true), kicked(false) {
			}

		private:
			bool kicked;
			bool done() const {
				Player::CPtr passee = Evaluation::nearest_friendly(world, Evaluation::calc_pass_positions(world).second);
				return kicked && (passee->has_ball() || Predicates::their_ball(world));
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players);
			}
			void execute() {
				kicked = false;
				std::pair<Point, Point> pp = Evaluation::calc_def_pass_positions(world);

				// dribble
				player->move(pp.first, (pp.second - player->position()).orientation(), Point());
				player->type(AI::Flags::MoveType::DRIBBLE);
				player->prio(AI::Flags::MovePrio::HIGH);
				kicked = Action::shoot_pass(world, player, pp.second);
			}
			std::string description() const {
				return "def-passer-shoot";
			}
	};

	class DefPasseeMove : public Tactic {
		public:
			DefPasseeMove(const World &world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				Point dest = Evaluation::calc_def_pass_positions(world).second;

				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
			}
			void execute() {
				Point dest = Evaluation::calc_def_pass_positions(world).second;
				Action::move(player, (world.ball().position() - player->position()).orientation(), dest);
		
				Player::CPtr passer = Evaluation::nearest_friendly(world, world.ball().position());

				bool fast_ball = world.ball().velocity().len() > negligible_velocity;				
				if(Action::within_angle_thresh(passer, dest, passer_tol_target) && passer->has_ball()){
					Point pass_dir(100, 0);
					pass_dir = pass_dir.rotate(passer->orientation());
					Point intercept_pos = closest_lineseg_point(player->position(), passer->position(), passer->position() + pass_dir);
					Point addit = passee_hack_dist*(intercept_pos - passer->position()).norm();
					Action::move(player, (passer->position() - intercept_pos).orientation(), intercept_pos + addit);
				} else if (!passer->has_ball()) { 
					if(fast_ball) {

						// if ball is moving away from robot not closer then we chase
						// if the ball is moving towards us then we want to intercept
						bool need_chase = (world.ball().position() - player->position()).dot(world.ball().velocity()) > 0;

						if(need_chase){
							Action::chase(world, player);
						}else{
							Point intercept_pos = closest_lineseg_point(player->position(), world.ball().position(), world.ball().position() + 100 * world.ball().velocity().norm());
							Point add = passee_hack_dist*(intercept_pos - player->position()).norm();
							Action::move(player, (-world.ball().velocity()).orientation(), intercept_pos + add);
						}

					} else {
						Action::chase(world, player);
					}

				} else {
					Action::move(player, (world.ball().position() - player->position()).orientation(), dest);
				}
				player->type(AI::Flags::MoveType::DRIBBLE);
			}
			std::string description() const {
				return "def-passee-move";
			}
	};
}


Tactic::Ptr AI::HL::STP::Tactic::passer_shoot_target(const World &world, Point target) {
	const Tactic::Ptr p(new PasserShootTarget(world, target));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_move_target(const World &world, Point target, bool active) {
	const Tactic::Ptr p(new PasseeMoveTarget(world, target, active));
	return p;
}


Tactic::Ptr AI::HL::STP::Tactic::passer_shoot(const World &world) {
	const Tactic::Ptr p(new PasserShoot(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_move(const World &world) {
	const Tactic::Ptr p(new PasseeMove(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::def_passer_shoot(const World &world) {
	const Tactic::Ptr p(new DefPasserShoot(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::def_passee_move(const World &world) {
	const Tactic::Ptr p(new DefPasseeMove(world));
	return p;
}

