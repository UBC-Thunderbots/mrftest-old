#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/move_spin.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/action/shoot.h"
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

	class SpinSteal : public Tactic {
		public:
			explicit SpinSteal(World world) : Tactic(world, true), none(false) {}

		private:
			bool none;
			bool done() const {
				return none;
			}
			Player select(const std::set<Player> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			Glib::ustring description() const {
				return u8"spin-steal";
			}
	};

	class BackUpSteal : public Tactic {
			public:
				explicit BackUpSteal(World world) : Tactic(world, true), state(BACKING_UP), finished(false), backup_dist(4 * Robot::MAX_RADIUS) {}

			private:
				enum state { BACKING_UP, GOING_FORWARD };

				state state;
				bool finished;
				Point start_pos;
				const double backup_dist;

				bool done() const {
					return finished && player.has_ball();
				}
				Player select(const std::set<Player> &players) const {
					return select_baller(world, players, player);
				}

				void player_changed() {
					start_pos = player.position();
				}

				void execute();

				Glib::ustring description() const {
					return u8"backup-steal";
				}
	};

	class TActiveDef : public Tactic {
		public:
			explicit TActiveDef(World world) : Tactic(world, true), finished(false) {}

		private:
			bool finished;
			bool done() const {
				return finished;
			}
			Player select(const std::set<Player> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			Glib::ustring description() const {
				return u8"tactive-def";
			}
	};

	class TDribbleToRegion : public Tactic {
		public:
			explicit TDribbleToRegion(World world, Region region_) : Tactic(world, true), region(region_) {}

		private:
			Region region;
			bool done() const {
				return region.inside(player.position());
			}
			Player select(const std::set<Player> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			Glib::ustring description() const {
				return u8"tdribble-to-region";
			}
	};

	class TSpinToRegion : public Tactic {
		public:
			explicit TSpinToRegion(World world, Region region_) : Tactic(world, true), region(region_) {}

		private:
			Region region;
			bool done() const {
				return region.inside(player.position());
			}
			Player select(const std::set<Player> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			Glib::ustring description() const {
				return u8"tspin-to-region";
			}
	};
}

void SpinSteal::execute() {
	none = false;
	EnemyTeam enemy = world.enemy_team();
	Point dirToBall = (world.ball().position() - player.position()).norm();
	for (const Robot i : enemy) {
		if (Evaluation::possess_ball(world, i)) {
			Action::move_spin(player, world.ball().position() + Robot::MAX_RADIUS * dirToBall);
			return;
		}
	}
	none = true;
}

void BackUpSteal::execute() {
	finished = (player.position() - start_pos).len() > backup_dist;

	switch(state) {
	case BACKING_UP:
		Action::move(world, player, start_pos + Point(-backup_dist, 0));
		if (finished) {
			state = GOING_FORWARD;
		}
		break;
	case GOING_FORWARD:
		Action::move(world, player, world.ball().position());
		if (player.has_ball()) {
			state = BACKING_UP;
		}
		break;
	}
}

void TActiveDef::execute() {
	finished = false;
	EnemyTeam enemy = world.enemy_team();

	for (const Robot i : enemy) {
		if (Evaluation::possess_ball(world, i)) {
			Point dirToBall = (world.ball().position() - i.position()).norm();
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

Tactic::Ptr AI::HL::STP::Tactic::spin_steal(World world) {
	Tactic::Ptr p(new SpinSteal(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::back_up_steal(World world) {
	Tactic::Ptr p(new BackUpSteal(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tactive_def(World world) {
	Tactic::Ptr p(new TActiveDef(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdribble_to_region(World world, Region region_) {
	Tactic::Ptr p(new TDribbleToRegion(world, region_));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tspin_to_region(World world, Region region_) {
	Tactic::Ptr p(new TSpinToRegion(world, region_));
	return p;
}

