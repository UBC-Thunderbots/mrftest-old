#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class PasserReady : public Tactic {
		public:
			PasserReady(const World &world, Coordinate p, Coordinate t) : Tactic(world), dest(p), target(t) {
			}

		private:
			Coordinate dest, target;
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				for (auto it = players.begin(); it != players.end(); ++it) {
					if ((*it)->has_ball()) {
						return *it;
					}
				}
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}
			void execute() {
#warning this is broken
				// TODO: fix this movement
				player->move(dest(), (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_MEDIUM);
				// orient towards target
				player->move(dest(), (target() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_MEDIUM);
				player->kick(7.5);
			}
	};

	class PasseeReady : public Tactic {
		public:
			// ACTIVE tactic!
			PasseeReady(const World &world, Coordinate p) : Tactic(world, true), dest(p) {
			}

		private:
			Coordinate dest;
			bool done() const {
				return (player->position() - dest()).len() < AI::HL::Util::POS_CLOSE;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
			}
			void execute() {
				player->move(dest(), (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_ready(const World &world, Coordinate pos, Coordinate target) {
	const Tactic::Ptr p(new PasserReady(world, pos, target));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_ready(const World &world, Coordinate pos) {
	const Tactic::Ptr p(new PasseeReady(world, pos));
	return p;
}

