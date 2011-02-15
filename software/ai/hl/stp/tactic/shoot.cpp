#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/ssm/move_ball.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Shoot : public Tactic {
		public:
			Shoot(World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool Shoot::done() const {
		return ssm_done();
	}

	Player::Ptr Shoot::select(const std::set<Player::Ptr> &players) const {
		for (auto it = players.begin(); it != players.end(); ++it) {
			if ((*it)->has_ball()) {
				return *it;
			}
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
	}

	void Shoot::execute() {
		set_ssm(AI::HL::STP::SSM::move_ball());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::shoot(World &world) {
	const Tactic::Ptr p(new Shoot(world));
	return p;
}

