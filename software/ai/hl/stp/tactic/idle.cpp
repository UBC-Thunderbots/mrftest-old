#include "ai/hl/stp/tactic/idle.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Idle : public Tactic {
		public:
			Idle(const World &world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	Player::Ptr Idle::select(const std::set<Player::Ptr> &players) const {
		return *players.begin();
	}

	void Idle::execute() {
		// move them to a particular place
		Point dest = Point(0, world.field().width() / 2);
		player->move(dest, (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::idle(World &world) {
	const Tactic::Ptr p(new Idle(world));
	return p;
}

