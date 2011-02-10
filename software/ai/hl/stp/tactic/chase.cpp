#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/ssm/get_ball.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Chase : public Tactic {
		public:
			Chase(World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			double score(Player::Ptr player) const;
			void execute();
	};

	bool Chase::done() const {
		return player->has_ball();
	}

	double Chase::score(Player::Ptr player) const {
		if (player->has_ball()) return 1e99;
		return -(player->position() - world.ball().position()).lensq();
	}

	void Chase::execute() {
		// if it has the ball, stay there
		if (player->has_ball()) {
			set_ssm(NULL);
			player->move(player->position(), player->orientation(), 0, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
			return;
		}

		// TODO: flags
		set_ssm(AI::HL::STP::SSM::get_ball());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::chase(World &world) {
	const Tactic::Ptr p(new Chase(world));
	return p;
}

