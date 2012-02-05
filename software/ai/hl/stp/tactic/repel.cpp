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
			Glib::ustring description() const {
				return "repel";
			}
	};

	bool Repel::done() const {
		return player && player->autokick_fired();
	}

	Player::Ptr Repel::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players, player);
	}

	void Repel::execute() {
		finished = false;
		finished = AI::HL::STP::Action::repel(world, player);
	}

	class CornerRepel : public Tactic {
		public:
			CornerRepel(const World &world) : Tactic(world, true), finished(false) {
			}

		private:
			bool finished;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			Glib::ustring description() const {
				return "corner_repel";
			}
	};

	bool CornerRepel::done() const {
		return player && player->autokick_fired();
	}

	Player::Ptr CornerRepel::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players, player);
	}

	void CornerRepel::execute() {
		finished = false;
		finished = AI::HL::STP::Action::corner_repel(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::repel(const World &world) {
	Tactic::Ptr p(new Repel(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::corner_repel(const World &world) {
	Tactic::Ptr p(new CornerRepel(world));
	return p;
}

