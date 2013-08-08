#include "ai/hl/stp/tactic/indirect_chip_to_cherry.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/indirect_cherry.h"
#include "ai/hl/stp/param.h"

#include "geom/angle.h"

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;

namespace {

	class IndirectChipToCherry : public Tactic {
		public:
			IndirectChipToCherry(World world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player select(const std::set<Player> &players) const;
			Point destination;
			void execute();
			void player_changed();
			Glib::ustring description() const {
				return u8"auto-chipped";
			}
	};

	bool IndirectChipToCherry::done() const {
		return Evaluation::cherry_at_point(world, player);
	}

	Player IndirectChipToCherry::select(const std::set<Player> &players) const {

		return select_baller(world, players, player);
	}

	void IndirectChipToCherry::player_changed() {
	}
	void IndirectChipToCherry::execute() {
		//see evaluation file for details
		Point destination = Evaluation::cherry_pivot(world);
		bool close = false;
		for (const Player i : world.friendly_team()) {
			if ((i.position() - destination).len() < Robot::MAX_RADIUS)
				close = true;
		}
		if (close) {
			Action::move(world, player, destination);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::indirect_chip_to_cherry(World world) {
	Tactic::Ptr p(new IndirectChipToCherry(world));
	return p;
}

