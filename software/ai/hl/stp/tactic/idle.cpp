#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/action/stop.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

namespace {
	class Idle final : public Tactic {
		public:
			explicit Idle(World world) : Tactic(world) {
			}

		private:
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override {
				return u8"idle";
			}
	};

	Player Idle::select(const std::set<Player> &players) const {
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

