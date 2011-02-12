#include "ai/hl/stp/tactic/steal.h"
#include "ai/hl/stp/ssm/get_ball.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Steal : public Tactic {
		public:
			Steal(World &world) : Tactic(world, true) {
			}

		private:
			bool done() const {
				return ssm_done();
			}

			double score(Player::Ptr player) const {
				if (player->has_ball()) return 1e99;
				return (player->position() - world.ball().position()).lensq();
			}

			void execute() {
				set_ssm(AI::HL::STP::SSM::get_ball());
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::steal(World &world) {
	const Tactic::Ptr p(new Steal(world));
	return p;
}

