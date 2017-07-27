#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/intercept.h"

#include "../action/dribble.h"
#include "ai/hl/stp/tactic/tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class Intercept final : public Tactic {
		public:
			explicit Intercept(World world) : Tactic(world) { }

		private:
			Player select(const std::set<Player> &players) const override;

			void execute(caller_t& ca) override;

			Glib::ustring description() const override {
				return u8"idle";
			}			
	};

	Player Intercept::select(const std::set<Player> &players) const {
		return select_baller(world, players, player());
	}

	void Intercept::execute(caller_t& ca) {
		while(true) {
			// if it has the ball, stay there
			if (player().has_ball()) {
				Action::dribble(ca, player());
			}else {
				// orient towards the enemy goal?
				Action::intercept(ca,world, player(), world.ball().position());
			}
			yield(ca);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::intercept(World world) {
	Tactic::Ptr p(new Intercept(world));
	return p;
}
