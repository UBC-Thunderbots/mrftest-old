#include "ai/hl/stp/tactic/move_active.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/pivot.h"
#include <algorithm>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;

namespace {
	class MoveActive final : public Tactic {
		public:
			explicit MoveActive(World world, const Point dest, const Angle orientation, const bool careful, const bool pivot) : Tactic(world, true), dest(dest), orientation(orientation), pivot(pivot), careful(careful) {
				arrived = false;
			}

		private:
			const Point dest;
			const Angle orientation;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			bool done() const override;
			void player_changed() override;
			bool arrived;
			bool pivot;
			bool careful;
			Glib::ustring description() const override {
				return u8"move-active";
			}
	};

	Player MoveActive::select(const std::set<Player> &players) const {
		return select_baller(world, players, player);
	}

	bool MoveActive::done() const {
		return (player.position() - dest).len() <= Robot::MAX_RADIUS;
	}

	void MoveActive::player_changed() {
	}

	void MoveActive::execute() {
		//if (pivot) Action::pivot(world, player, dest, 0.15);
		if (careful) Action::move_careful(world, player, dest);
		else Action::move(player, orientation, dest);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_active(World world, const Point dest, const Angle orientation, const bool pivot, const bool careful) {
	Tactic::Ptr p(new MoveActive(world, dest, orientation, pivot, careful));
	return p;
}

