#include "ai/hl/stp/tactic/direct_free_friendly_pivot.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"

#include "geom/angle.h"

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {

	class DirectFreeFriendlyPivot : public Tactic {
		public:
			DirectFreeFriendlyPivot(World world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player select(const std::set<Player> &players) const;
			void execute();
			void player_changed();
			Glib::ustring description() const {
				return u8"auto-chipped";
			}

	};

	bool DirectFreeFriendlyPivot::done() const {
		return player.autokick_fired();
	}

	Player DirectFreeFriendlyPivot::select(const std::set<Player> &players) const {
		return select_baller(world, players, player);
	}

	void DirectFreeFriendlyPivot::player_changed() {
	}

	//run at ball with auto chip on. when it passes sensors, it auto chips
	void DirectFreeFriendlyPivot::execute() {
		player.move(world.ball().position(), (world.ball().position() - player.position()).orientation(), Point());
		player.autochip(0.7);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::direct_free_friendly_pivot(World world) {
	Tactic::Ptr p(new DirectFreeFriendlyPivot(world));
	return p;
}

