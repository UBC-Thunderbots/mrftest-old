#include "ai/hl/stp/tactic/move_wait_playtype.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class MoveWaitPlaytype : public Tactic {
		public:
			MoveWaitPlaytype(const World &world, const Coordinate dest, const PlayType::PlayType playtype) : Tactic(world, true), dest(dest), playtype(playtype) {
			}

		private:
			const Coordinate dest;
			const PlayType::PlayType playtype;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool MoveWaitPlaytype::done() const {
		return world.playtype() == playtype && (player->position() - dest()).len() < AI::HL::Util::POS_CLOSE;
	}

	Player::Ptr MoveWaitPlaytype::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
	}

	void MoveWaitPlaytype::execute() {
		player->move(dest(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_wait_playtype(const World &world, const Coordinate dest, const PlayType::PlayType playtype) {
	const Tactic::Ptr p(new MoveWaitPlaytype(world, dest, playtype));
	return p;
}

