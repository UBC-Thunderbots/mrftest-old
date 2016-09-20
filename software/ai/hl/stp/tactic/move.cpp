#include <algorithm>

#include "ai/hl/stp/tactic/move.h"
#include "util/dprint.h"
#include "ai/hl/util.h"

namespace Primitives = AI::BE::Primitives;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class MoveOnce final : public Tactic {
		public:
			explicit MoveOnce(World world, Point dest) : Tactic(world), dest(dest) {
			}

		private:
			const Point dest;
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"move-once";
			}
	};

	Player MoveOnce::select(const std::set<Player> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest));
	}

	void MoveOnce::execute(caller_t& caller) {
		Primitives::Move move(player(), dest);
		wait(caller, move);
	}

	class Move final : public Tactic {
		public:
			explicit Move(World world, const Coordinate dest) : Tactic(world), dest(dest) {
			}

		private:
			const Coordinate dest;
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"move";
			}
	};

	Player Move::select(const std::set<Player> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest.position()));
	}

	void Move::execute(caller_t& caller) {
		while (true) {
			Primitives::Move move(player(), dest.position());
			yield(caller);
			if (move.done()) break;
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_once(World world, Point dest) {
	return Tactic::Ptr(new MoveOnce(world, dest));
}
Tactic::Ptr AI::HL::STP::Tactic::move(World world, Coordinate dest) {
	return Tactic::Ptr(new Move(world, dest));
}
