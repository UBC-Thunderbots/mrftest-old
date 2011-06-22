#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;

namespace {
	class PasserShoot : public Tactic {
		public:
			PasserShoot(const World &world) : Tactic(world, true), kicked(false) {
			}

		private:
			bool kicked;
			bool done() const {
				return kicked;
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
		private:
			void on_player_removed(std::size_t index) {
				if( world.friendly_team().get(index) == Player::CPtr(passer)){
					passer.reset();
				}
			}

		public:
			PasserShootTarget(const World &world, Point target) : Tactic(world, true), kicked(false), target(target) {
					world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &PasserShootTarget::on_player_removed));
			}

		private:
			bool kicked;
			Point target;
			mutable Player::Ptr passer;

			bool done() const {
				return kicked;
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				if(passer.is()){
					return passer;
				}
				passer = select_baller(world, players);
				return passer;
			}

			void execute() {
			}

			std::string description() const {
				return "passer-shoot-target";
			}
	};

	class PasseeMoveTarget : public Tactic, public sigc::trackable {

		public:
			PasseeMoveTarget(const World &world, Point target) : Tactic(world), target(target) {
				world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &PasseeMoveTarget::on_player_removing));
			}

		private:
			mutable Player::Ptr passee;
			Point target;

			void on_player_removing(std::size_t index) {
				if( world.friendly_team().get(index) == Player::CPtr(passee)){
					passee.reset();
				}
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				if(passee.is()){
					return passee;
				}
				passee = *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(target)) ;
				return passee;
			}
			void execute() {
				// Action specific to this tactic
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
				return kicked;
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

Tactic::Ptr AI::HL::STP::Tactic::passee_move_target(const World &world, Point target) {
	const Tactic::Ptr p(new PasseeMoveTarget(world, target));
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

