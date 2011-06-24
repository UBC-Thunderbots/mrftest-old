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
	DoubleParam negligible_velocity("velocity to ignore", "STP/Tactic/pass", 0.1, 0.0, 1.0);
	DoubleParam passee_hack_dist("Hack to get reciever to move more quickly to intercept pos by modifying dest (meters)", "STP/Tactic/pass", 0.0, 0.0, 1.0);
//	double passer_tol_target = 30.0; 
//	double negligible_velocity = 0.1;
//	double passee_hack_dist = 0.0;
	
	struct kick_info{
		Point kicker_location;
		double kicker_orientation;
		Point kicker_target;
		bool kicked;
	} ;

	class PasserShoot : public Tactic {
		public:
			PasserShoot(const World &world, bool defensive) : Tactic(world, true), dynamic(true), defensive(defensive), kicked(false) {
			}

			PasserShoot(const World &world, Coordinate target, bool defensive) : Tactic(world, true), dynamic(false), defensive(defensive), target(target), kicked(false) {
			}
			static kick_info passer_info;
			
		private:
			bool dynamic;
			bool kicked;
			bool defensive;
			Coordinate target;

			bool done() const {
#warning TODO allow more time
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();				
				return kicked && (player->position() - world.ball().position()).len() > (player->position() - dest).len()/2;
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players);
			}

			void execute() {
				kicked = kicked || player->autokick_fired();
				if(!player->autokick_fired() && !kicked){
					PasserShoot::passer_info.kicker_location = player->position();
					PasserShoot::passer_info.kicker_orientation = player->orientation();
					PasserShoot::passer_info.kicker_target = target.position();
					PasserShoot::passer_info.kicked = false;
				} else {
					PasserShoot::passer_info.kicked = true;
				}
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();
				Player::CPtr passee = Evaluation::nearest_friendly(world, dest);
				kicked = Action::shoot_pass(world, player, dest);
			}

			std::string description() const {
#warning TODO give more information
				return "passer-shoot";
			}
	};
	
	kick_info PasserShoot::passer_info;

	class PasseeMove : public Tactic {
		public:
			PasseeMove(const World &world, bool defensive) : Tactic(world, false), dynamic(true), defensive(defensive), target(target) {
			}

			PasseeMove(const World &world, Coordinate target, bool defensive) : Tactic(world, false), dynamic(false), defensive(defensive), target(target) {
			}

		private:
			bool dynamic;
			bool defensive;
			Coordinate target;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();

				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
			}

			void execute() {
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();
				Player::CPtr passer = Evaluation::nearest_friendly(world, world.ball().position());
				kick_info passer_info = PasserShoot::passer_info;
				bool fast_ball = world.ball().velocity().len() > negligible_velocity;
				if((Action::within_angle_thresh(passer_info.kicker_location, passer_info.kicker_orientation, passer_info.kicker_target, passer_tol_target) && !passer_info.kicked) || (passer_info.kicked && !fast_ball)){
					Point pass_dir(100, 0);
					pass_dir = pass_dir.rotate(passer_info.kicker_orientation);

					Point intercept_pos = closest_lineseg_point(player->position(), passer_info.kicker_location, passer_info.kicker_location + pass_dir);
					Point addit = passee_hack_dist*(intercept_pos - player->position()).norm();
					Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos + addit);
				} else if(passer_info.kicked && fast_ball){
					Point intercept_pos = closest_lineseg_point(player->position(), world.ball().position(),  world.ball().position() + 100 * (world.ball().velocity().norm()));
					Point pass_dir = (world.ball().position() - passer_info.kicker_location).norm();
					Point addit = passee_hack_dist*(intercept_pos - player->position()).norm();
					Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos + addit);
				}else{
					Action::move(player, (world.ball().position() - player->position()).orientation(), passer_info.kicker_target);
				}
			}

			std::string description() const {
				return "passee-target";
			}
	};
	
	
class PasseeRecieve : public Tactic {
		public:
			PasseeRecieve(const World &world, bool defensive) : Tactic(world, true), dynamic(true), defensive(defensive), target(target) {
			}

			PasseeRecieve(const World &world, Coordinate target, bool defensive) : Tactic(world, true), dynamic(false), defensive(defensive), target(target) {
			}

		private:
			bool dynamic;
			bool defensive;
			Coordinate target;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
			}

			void execute() {
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();
				kick_info passer_info = PasserShoot::passer_info;
				bool fast_ball = world.ball().velocity().len() > negligible_velocity;
				if(!fast_ball){
					Point pass_dir(100, 0);
					pass_dir = pass_dir.rotate(passer_info.kicker_orientation);

					Point intercept_pos = closest_lineseg_point(player->position(), passer_info.kicker_location, passer_info.kicker_location + pass_dir);
					Point addit = passee_hack_dist*(intercept_pos - player->position()).norm();
					Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos + addit);
				} else {
					Point intercept_pos = closest_lineseg_point(player->position(), world.ball().position(),  world.ball().position() + 100 * (world.ball().velocity().norm()));
					Point pass_dir = (world.ball().position() - passer_info.kicker_location).norm();
					Point addit = passee_hack_dist*(intercept_pos - player->position()).norm();
					Action::move(player, (passer_info.kicker_location - intercept_pos).orientation(), intercept_pos + addit);
				}
			}

			std::string description() const {
				return "passee-target-recieve";
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_shoot_target(const World &world, Coordinate target) {
	const Tactic::Ptr p(new PasserShoot(world, target, false));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_move_target(const World &world, Coordinate target) {
	const Tactic::Ptr p(new PasseeMove(world, target, false));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passer_shoot_dynamic(const World &world) {
	const Tactic::Ptr p(new PasserShoot(world, false));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_move_dynamic(const World &world) {
	const Tactic::Ptr p(new PasseeMove(world, false));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::def_passer_shoot(const World &world) {
	const Tactic::Ptr p(new PasserShoot(world, true));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::def_passee_move(const World &world) {
	const Tactic::Ptr p(new PasseeMove(world, true));
	return p;
}

