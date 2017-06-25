#include <algorithm>

#include "ai/hl/stp/tactic/test_tactics.h"
#include "util/dprint.h"
#include "ai/hl/util.h"

namespace Primitives = AI::BE::Primitives;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
    
	class MoveTest final : public Tactic {
		public:
			explicit MoveTest(World world, Point dest) : Tactic(world), dest(dest) {
			}

		private:
			const Point dest;
            Point original_pos;
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"move test";
			}
	};

	Player MoveTest::select(const std::set<Player> &players) const {
		Player p = *(players.begin());
		return p;
	}

	void MoveTest::execute(caller_t& ca) {
        original_pos = player().position();
		AI::HL::STP::Action::move_rrt(ca, world, player(), dest);
        AI::HL::STP::Action::move_rrt(ca, world, player(), original_pos);
	}

	class MoveTestOrientation final : public Tactic {
		public:
			explicit MoveTestOrientation(World world, Point dest) : Tactic(world), dest(dest) {
			}

		private:
			const Point dest;
            Point original_pos;
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"move test";
			}
	};

	Player MoveTestOrientation::select(const std::set<Player> &players) const {
		Player p = *(players.begin());
		return p;
	}

	void MoveTestOrientation::execute(caller_t& ca) {
        original_pos = player().position();
		AI::HL::STP::Action::move_rrt(ca, world, player(), dest);
		AI::HL::STP::Action::move_rrt(ca, world, player(), original_pos);
	}
}


Tactic::Ptr AI::HL::STP::Tactic::move_test(World world, Point dest) {
	return Tactic::Ptr(new MoveTest(world, dest));
}

Tactic::Ptr AI::HL::STP::Tactic::move_test_orientation(World world, Point dest) {
	return Tactic::Ptr(new MoveTestOrientation(world, dest));
}
