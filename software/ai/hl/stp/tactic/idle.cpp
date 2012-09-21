#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/action/stop.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

namespace {
	class Idle : public Tactic {
		public:
			Idle(World world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			Glib::ustring description() const {
				return "idle";
			}
	};

	Player::Ptr Idle::select(const std::set<Player::Ptr> &players) const {
		return *players.begin();
	}

	void Idle::execute() {
		Action::stop(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::idle(World world) {
	Tactic::Ptr p(new Idle(world));
	return p;
}

