#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/old/tactics.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Chase : public Tactic {
		public:
			Chase(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool Chase::done() const {
		return player->has_ball();
	}

	Player::Ptr Chase::select(const std::set<Player::Ptr> &players) const {
		for (auto it = players.begin(); it != players.end(); ++it) {
			if ((*it)->has_ball()) {
				return *it;
			}
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
	}

	void Chase::execute() {
		// if it has the ball, stay there
		if (player->has_ball()) {
			player->move(player->position(), player->orientation(), 0, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
			return;
		}

		// TODO: flags
#warning use chase SSM
		AI::HL::Tactics::chase(world, player, 0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::chase(const World &world) {
	const Tactic::Ptr p(new Chase(world));
	return p;
}

