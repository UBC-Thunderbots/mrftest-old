#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/ssm/move_ball.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

// shoot

namespace {
	class Shoot : public Tactic {
		public:
			Shoot(World &world) : Tactic(world, true) {
			}

		private:
			bool done() const {
#warning find a way to check that the ball has left off in the right direction
				return true;
			}

			double score(Player::Ptr player) const {
				if (player->has_ball()) return 1e99;
				return -(player->position() - world.ball().position()).lensq();
			}

			void execute() {
				// TODO: flags
				set_ssm(AI::HL::STP::SSM::move_ball());
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::shoot(World &world) {
	const Tactic::Ptr p(new Shoot(world));
	return p;
}

