#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include <algorithm>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;

namespace {
	class ShadowKickoff : public Tactic {
		public:
			ShadowKickoff(const World &world, Enemy::Ptr enemy, const Coordinate default_loc) : Tactic(world), enemy(enemy), default_loc(default_loc) {
			}

		private:
			const Enemy::Ptr enemy;
			const Coordinate default_loc;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "shadow kickoff";
			}
	};

	Player::Ptr ShadowKickoff::select(const std::set<Player::Ptr> &players) const {
		Point location_eval;
		if (enemy->evaluate().is()) {
			location_eval = enemy->evaluate()->position();
		} else {
			location_eval = default_loc.position();
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(location_eval));
	}

	void ShadowKickoff::execute() {
		if (enemy->evaluate().is()) {
			Action::move(world, player, enemy->evaluate()->position() + Point(-0.3, 0));
		} else {
			Action::move(world, player, default_loc.position());
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_kickoff(const World &world, Enemy::Ptr enemy, const Coordinate default_loc) {
	const Tactic::Ptr p(new ShadowKickoff(world, enemy, default_loc));
	return p;
}

