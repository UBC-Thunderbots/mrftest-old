#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class PenaltyShoot : public Tactic {
		public:
			PenaltyShoot(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool PenaltyShoot::done() const {
#warning TODO
		return !player->has_ball();
	}

	Player::Ptr PenaltyShoot::select(const std::set<Player::Ptr> &players) const {
		for (auto it = players.begin(); it != players.end(); ++it) {
			if ((*it)->has_ball()) {
				return *it;
			}
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
	}

	void PenaltyShoot::execute() {
#warning this tactic should decide when to shoot
#warning use shoot SSM
		//set_ssm(AI::HL::STP::SSM::move_ball());
		AI::HL::STP::Actions::shoot(world, player, 0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_shoot(const World &world) {
	const Tactic::Ptr p(new PenaltyShoot(world));
	return p;
}

