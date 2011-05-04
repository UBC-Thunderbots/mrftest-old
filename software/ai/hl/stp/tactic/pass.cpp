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
				std::pair <Point, Point> pp = Evaluation::calc_pass_positions(world);
				
				// orient towards target
				Action::move(player, (pp.second - player->position()).orientation(), pp.first);
				kicked = Action::shoot(world,player,pp.second);

			}
			std::string description() const {
				return "passer-shoot";
			}
			
	};

	class PasseeReceive : public Tactic {
		public:
			// ACTIVE tactic!
			PasseeReceive(const World &world) : Tactic(world, true) {
			}

		private:

			bool done() const {
				return player->has_ball();
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
	
				Point dest = Evaluation::calc_pass_positions(world).second;

				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
			}
			void execute() {
				Point dest = Evaluation::calc_pass_positions(world).second;
				Action::move(player, (world.ball().position() - player->position()).orientation(), dest);
			}
			std::string description() const {
				return "passee-receive";
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
				std::pair <Point, Point> pp = Evaluation::calc_def_pass_positions(world);
				
				// orient towards target
				Action::move(player, (pp.second - player->position()).orientation(), pp.first);
				kicked = Action::shoot(world,player,pp.second);

			}
			std::string description() const {
				return "def-passer-shoot";
			}
			
	};
	
	class DefPasseeReceive : public Tactic {
		public:
			// ACTIVE tactic!
			DefPasseeReceive(const World &world) : Tactic(world, true) {
			}

		private:

			bool done() const {
				return player->has_ball();
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
	
				Point dest = Evaluation::calc_def_pass_positions(world).second;

				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
			}
			void execute() {
				Point dest = Evaluation::calc_def_pass_positions(world).second;
				Action::move(player, (world.ball().position() - player->position()).orientation(), dest);
			}
			std::string description() const {
				return "def-passee-receive";
			}
			
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_shoot(const World &world) {
	const Tactic::Ptr p(new PasserShoot(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_receive(const World &world) {
	const Tactic::Ptr p(new PasseeReceive(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::def_passer_shoot(const World &world) {
	const Tactic::Ptr p(new DefPasserShoot(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::def_passee_receive(const World &world) {
	const Tactic::Ptr p(new DefPasseeReceive(world));
	return p;
}

