#include "ai/hl/stp/tactic/chip.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/shoot.h"
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

	class ChipTarget : public Tactic {
		public:
			ChipTarget(World world, const Coordinate target) : Tactic(world, true), target(target), kick_attempted(false) {
			}

		private:
			Coordinate target;
			bool kick_attempted;
			bool done() const;
			Player select(const std::set<Player> &players) const;
			void execute();
			void player_changed();
			Glib::ustring description() const {
				return u8"chip-target";
			}
	};

	bool ChipTarget::done() const {
		return player /* && kick_attempted */ && player.autokick_fired();
	}

	Player ChipTarget::select(const std::set<Player> &players) const {
		// if a player attempted to shoot, keep the player
		Player player_c = player;
		if (players.count(player) && Evaluation::possess_ball(world, player_c) && player.has_chipper()) {
			return player;
		}
		if (kick_attempted && players.count(player)) {
			return player;
		}
		return select_baller(world, players, player);
	}

	void ChipTarget::player_changed() {
		kick_attempted = false;
	}

	void ChipTarget::execute() {
		if (AI::HL::STP::Action::chip_target(world, player, target.position())) {
			kick_attempted = true;
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::chip_target(World world, const Coordinate target) {
	Tactic::Ptr p(new ChipTarget(world, target));
	return p;
}

