#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Coordinate;

using AI::HL::STP::min_pass_dist;

namespace {

	DoubleParam passer_tol_target("when passer within this angle tol passee responds to passer direction", "STP/Tactic/pass", 30.0, 0.0, 180.0);
	DoubleParam fast_velocity("velocity of pass threshold", "STP/Tactic/pass", 1.0, 0.0, 1.0);
	DoubleParam negligible_velocity("velocity to ignore", "STP/Tactic/pass", 0.05, 0.0, 1.0);
	DoubleParam passee_hack_dist("Hack to get reciever to move more quickly to intercept pos by modifying dest (meters)", "STP/Tactic/pass", 0.0, 0.0, 1.0);
	DoubleParam passee_hack_appl("dist from target when passee hack is applicable (meters)", "STP/Tactic/pass", 0.1, 0.001, 1.0);
	DoubleParam ball_region_param(" the radius (meters) in which passer must be with repect to ball before valid ", "STP/Tactic/pass", 1.0, 0.0, 5.0);
	BoolParam passer_depends_baller_can_shoot(" shot on net avaiable means that Passer should fail ", "STP/Tactic/pass", true);
	BoolParam passer_depends_calc_best_shot_target(" pass blocked means that Passer should fail ", "STP/Tactic/pass", true);

	struct kick_info {
		Point kicker_location;
		double kicker_orientation;
		Point kicker_target;
		bool kicked;
	};

	class PasserShoot : public Tactic {
		public:
			PasserShoot(const World &world) : Tactic(world, true), dynamic(true), kicked(false) {
				passer_info.kicked = false;
			}

			PasserShoot(const World &world, Coordinate target) : Tactic(world, true), dynamic(false), target(target), kicked(false) {
				passer_info.kicked = false;
			}
			static kick_info passer_info;
			
		private:
			bool dynamic;
			bool kicked;
			Coordinate target;

			bool done() const {
#warning TODO allow more time
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();				
				//return kicked && (player->position() - world.ball().position()).len() > (player->position() - dest).len()/4;

#warning WTH how this works
				return kicked || player->autokick_fired();
			}
			
			bool fail() const {
				return player.is()
					// should fail when cannot pass to target,
					&& (!passer_depends_calc_best_shot_target
							|| Evaluation::can_pass(world, player->position(), target.position()))
					// or a shot on net is available
					&& (!passer_depends_baller_can_shoot
							|| AI::HL::STP::Predicates::baller_can_shoot(world));
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


				///////////////////////////////////////////////////////////////////////////////////////////////////////
				// calculate the largest circle centered at the passers target                                       //
				// that is inscribed by the two rays that represents the passers allowed error                       //
				// passee finds the most appropiate location within this circle to adapt to the passers orientation  //
				///////////////////////////////////////////////////////////////////////////////////////////////////////

				// target normalized to position of passer
				Point passer_target = passer_info.kicker_target - passer_info.kicker_location;
				double pass_thresh = AI::HL::STP::Action::pass_threshold * M_PI/180.0;

				// the intersection point of largest circle inscribed by 2 rays
				// (the rays are the max and min angle that is acceptable for passer
				// intersect A & B are the intersection point of this circle and these two rays
				Point circle_outside_ray_intersectA = closest_lineseg_point(passer_info.kicker_target, passer_info.kicker_location,  passer_info.kicker_location + passer_target.rotate(pass_thresh) );
				Point circle_outside_ray_intersectB = closest_lineseg_point(passer_info.kicker_target, passer_info.kicker_location,  passer_info.kicker_location + passer_target.rotate(-pass_thresh) );
				double target_radius = (circle_outside_ray_intersectA - passer_info.kicker_target).len();


				//
				// if the passee is outside the acceptable region for recieving a pass then make it go to an
				// appropiate spot that is inside that region
				//
				Point passer_dir(100, 0);
				passer_dir = passer_dir.rotate(passer_info.kicker_orientation);

				// vector of Points where passer ray intersects circle described above
				std::vector<Point> Passer_ray_target_region_intersect = line_circle_intersect(passer_info.kicker_target, target_radius, passer_info.kicker_location, passer_info.kicker_location + passer_dir);

				if( Passer_ray_target_region_intersect.size() == 0 ){//passer is not within passing tolerance
					double distA = closest_lineseg_point(circle_outside_ray_intersectA, passer_info.kicker_location , passer_info.kicker_location + passer_dir).len();
					double distB =  closest_lineseg_point(circle_outside_ray_intersectB, passer_info.kicker_location , passer_info.kicker_location + passer_dir).len();
					Point passee_goto= circle_outside_ray_intersectA;
					if( distB < distA){
						passee_goto= circle_outside_ray_intersectB;
					}

					// Additional distance to add to location in the case that we are dealing with an improperly
					// tuned controller
					Point add_dist_hack(0,0);
					// if the passee is currently outside of it's target by certain amount add hack distance 
					if((passee_goto - player->position()).len() > passee_hack_appl){
						add_dist_hack = passee_hack_dist*(passee_goto - player->position()).norm();
					}
					passee_goto = passee_goto + add_dist_hack;
					Action::move(player, (world.ball().position() - player->position()).orientation(), passee_goto);
				} else { //passer is within passer tolerance
					Point passee_goto(0.0,0.0);
					for(unsigned int i = 0; i < Passer_ray_target_region_intersect.size(); i++){
						passee_goto+=Passer_ray_target_region_intersect[i];
					}
					passee_goto/=Passer_ray_target_region_intersect.size();

					// Additional distance to add to location in the case that we are dealing with an improperly
					// tuned controller
					Point add_dist_hack(0,0);
					// if the passee is currently outside of it's target by certain amount add hack distance 
					if((passee_goto - player->position()).len() > passee_hack_appl){
						add_dist_hack = passee_hack_dist*(passee_goto - player->position()).norm();
					}
					passee_goto = passee_goto + add_dist_hack;
					Action::move(player, (world.ball().position() - player->position()).orientation(), passee_goto);
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
				return player.is() && player->has_ball();
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

	class PasserRandom : public Tactic {
		public:
			PasserRandom(const World &world) : Tactic(world, true), kick_attempted(false) {
			}

		private:
			bool kick_attempted;
			Player::CPtr target;

			bool done() const {
				return player.is() && kick_attempted && player->autokick_fired();
			}

			bool fail() const {
				if (!Evaluation::find_random_passee(world).is()) {
					return true;
				}

				if (AI::HL::STP::Predicates::baller_can_shoot(world)) {
					return true;
				}

				return false;
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				// if a player attempted to shoot, keep the player
				if (kick_attempted && players.count(player)) {
					return player;
				}
				return select_baller(world, players);
			}

			void execute() {
				if (target.is() && !Evaluation::passee_suitable(world, target)) {
					target.reset();
				}

				if (!target.is()) {
					target = Evaluation::find_random_passee(world);
				}

				if (!target.is()) {
					// should fail
					return;
				}

				kick_attempted = kick_attempted || Action::shoot_pass(world, player, target);
			}

			std::string description() const {
				return "passer-random";
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

Tactic::Ptr AI::HL::STP::Tactic::passer_random(const World &world) {
	const Tactic::Ptr p(new PasserRandom(world));
	return p;
}

