#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/move_spin.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;
using AI::HL::STP::Coordinate;
using AI::HL::STP::Region;

namespace {

	class TSteal : public Tactic {
		public:
			TSteal(const World &world) : Tactic(world, true), none(false) {}

		private:
			bool none;
			bool done() const {
				return none;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			Glib::ustring description() const {
				return "tsteal";
			}
	};

	class TActiveDef : public Tactic {
		public:
			TActiveDef(const World &world) : Tactic(world, true), finished(false) {}

		private:
			bool finished;
			bool done() const {
				return finished;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			Glib::ustring description() const {
				return "tactive_def";
			}
	};

	class TDribbleToRegion : public Tactic {
		public:
			TDribbleToRegion(const World &world, Region region_) : Tactic(world, true), region(region_) {}

		private:
			Region region;
			bool done() const {
				return region.inside(player->position());
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			Glib::ustring description() const {
				return "tdribble_to_region";
			}
	};

	class TSpinToRegion : public Tactic {
		public:
			TSpinToRegion(const World &world, Region region_) : Tactic(world, true), region(region_) {}

		private:
			Region region;
			bool done() const {
				return region.inside(player->position());
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			Glib::ustring description() const {
				return "tspin_to_region";
			}
	};
}

void TSteal::execute() {
	none = false;
	const EnemyTeam &enemy = world.enemy_team();
	Point dirToBall = (world.ball().position() - player->position()).norm();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (Evaluation::possess_ball(world, enemy.get(i))) {
			Action::move_spin(player, world.ball().position() + Robot::MAX_RADIUS * dirToBall);
			return;
		}
	}
	none = true;
}

void TActiveDef::execute() {
	finished = false;
	const EnemyTeam &enemy = world.enemy_team();

	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (Evaluation::possess_ball(world, enemy.get(i))) {
			Point dirToBall = (world.ball().position() - enemy.get(i)->position()).norm();
			Action::move_spin(player, world.ball().position() + 0.75 * Robot::MAX_RADIUS * dirToBall);
			return;
		}
	}

	finished = Action::repel(world, player);
}

void TDribbleToRegion::execute() {
	Action::dribble(world, player, region.center_position());
}

void TSpinToRegion::execute() {
	Action::move_spin(player, region.center_position());
}

Tactic::Ptr AI::HL::STP::Tactic::tsteal(const World &world) {
	Tactic::Ptr p(new TSteal(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tactive_def(const World &world) {
	Tactic::Ptr p(new TActiveDef(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdribble_to_region(const World &world, Region region_) {
	Tactic::Ptr p(new TDribbleToRegion(world, region_));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tspin_to_region(const World &world, Region region_) {
	Tactic::Ptr p(new TSpinToRegion(world, region_));
	return p;
}

