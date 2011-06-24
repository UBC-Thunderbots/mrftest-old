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

	class PasserShoot : public Tactic {
		public:
			PasserShoot(const World &world, bool defensive) : Tactic(world, true), dynamic(true), defensive(defensive) {
			}

			PasserShoot(const World &world, Coordinate target, bool defensive) : Tactic(world, true), dynamic(false), defensive(defensive), target(target) {
			}

		private:
			bool dynamic;
			bool kicked;
			bool defensive;
			Coordinate target;

			bool done() const {
#warning TODO allow more time
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();				
				return kicked && (player->position() - world.ball().position()).len() > (player->position() - dest).len()/2;
				//return kicked;
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players);
			}

			void execute() {
				kicked = false;
				Point dest = dynamic ? Evaluation::passee_position(world) : target.position();

				Player::CPtr passee = Evaluation::nearest_friendly(world, dest);

#warning TODO find a good location for passer
				Point target = Evaluation::passer_position(world, dest, defensive);
				Action::move(world, player, target);
				kicked = Action::shoot_pass(world, player, dest);
			}

			std::string description() const {
#warning TODO give more information
				return "passer-shoot";
			}
	};

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

				Action::move(world, player, dest);
			}

			std::string description() const {
				return "passee-target";
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

