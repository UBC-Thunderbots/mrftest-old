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
	class MoveActive : public Tactic {
		public:
			MoveActive(World world, const Point dest, const Angle orientation, const bool pivot) : Tactic(world, true), dest(dest), orientation(orientation), pivot(pivot) {
				arrived = false;
			}

		private:
			const Point dest;
			const Angle orientation;
			Player select(const std::set<Player> &players) const;
			void execute();
			bool done() const;
			void player_changed();
			bool arrived;
			bool pivot;
			Glib::ustring description() const {
				return "move-active";
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
		if (pivot) Action::pivot(world, player, dest, 0.15);
		else Action::move(player, orientation, dest);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_active(World world, const Point dest, const Angle orientation, const bool pivot) {
	Tactic::Ptr p(new MoveActive(world, dest, orientation, pivot));
	return p;
}

