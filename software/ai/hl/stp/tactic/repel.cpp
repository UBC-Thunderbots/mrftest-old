#include "ai/hl/stp/tactic/repel.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Repel : public Tactic {
		public:
			Repel(const World &world) : Tactic(world, true), finished(false) {
			}

		private:
			bool finished;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "repel";
			}
	};

	bool Repel::done() const {
		/*
#warning TODO
		// the ball is in some safe state:
		// far away from goalie
		// not rolling towards the defense area at all
		if (world.ball().position().x > 0) {
			return true;
		}
		return false;
		*/
		return player.is() && player->autokick_fired();
	}

	Player::Ptr Repel::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players, player);
	}

	void Repel::execute() {
		finished = false;
		finished = AI::HL::STP::Action::repel(world, player);
		player->flags(0);
	}
	
	class CornerRepel : public Tactic {
		public:
			CornerRepel(const World &world) : Tactic(world, true) {
			}

		private:
			bool finished;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "corner_repel";
			}
	};

	bool CornerRepel::done() const {
		return player.is() && player->autokick_fired();
	}

	Player::Ptr CornerRepel::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players, player);
	}

	void CornerRepel::execute() {
		finished = false;
		finished = AI::HL::STP::Action::corner_repel(world, player);
		player->flags(0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::repel(const World &world) {
	const Tactic::Ptr p(new Repel(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::corner_repel(const World &world) {
	const Tactic::Ptr p(new CornerRepel(world));
	return p;
}

