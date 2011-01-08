#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

// defend goal

namespace {
	class DefendGoal : public Tactic {
		public:
			DefendGoal(World &world) : Tactic(world) {
			}

		private:
			double score(Player::Ptr player) const {
				if (world.friendly_team().get(0) == player) {
					return 1;
				}
				return 0;
			}

			void execute() {
				// TODO: use proper skill
				AI::HL::Tactics::lone_goalie(world, player);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::defend_goal(World &world) {
	const Tactic::Ptr p(new DefendGoal(world));
	return p;
}

// repel

namespace {
	class Repel : public Tactic {
		public:
			Repel(World &world) : Tactic(world) {
			}

		private:
			bool done() const {
				// will never be done... unless ball is outside the field
				return false;
			}

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - world.ball().position()).len());
			}

			void execute() {
				// TODO: use proper skill
				AI::HL::Tactics::repel(world, player, 0);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::repel(World &world) {
	const Tactic::Ptr p(new Repel(world));
	return p;
}

