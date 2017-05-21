#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/action/stop.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

//FILE DESCRIPTION: Renders robot idle.

namespace {
	class Idle final : public Tactic {
		public:
			explicit Idle(World world) : Tactic(world) { }

// REFERENCE: Tactic superclass for override methods descriptions.

		private:
			
			Player select(const std::set<Player> &players) const override;

			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"idle";
			}
	};


	Player Idle::select(const std::set<Player> &players) const {
		return *players.begin(); // returns first element/robot from player vector
	}

// executes caller on selected player

	void Idle::execute(caller_t& caller) {
		while (true) {
			caller();
		}
	}
}
// creates idle world for selected player by calling superclass
Tactic::Ptr AI::HL::STP::Tactic::idle(World world) {
	return Tactic::Ptr(new Idle(world));
}
