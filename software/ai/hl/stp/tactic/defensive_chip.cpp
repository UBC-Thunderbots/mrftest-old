#include <algorithm>

#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/indirect_chip.h"
#include "ai/hl/stp/tactic/defensive_chip.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/enemy.h"

namespace Primitives = AI::BE::Primitives;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
using AI::HL::STP::Coordinate;
using namespace AI::HL::Util;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;


namespace {
	class DefensiveChip final : public Tactic {
		public:
			explicit DefensiveChip(World world) : Tactic(world) { //constructor
			}

		private:
			void execute(caller_t& ca) override;
			Player select(const std::set<Player> &players) const override;

			Glib::ustring description() const override {
				return u8"defensive chip";
			}
	};

	Player DefensiveChip::select(const std::set<Player> &players) const {
		return select_baller(world, players, player());
	}

	void DefensiveChip::execute(caller_t& ca) {
		while(true) {
			std::vector<Point> obstacles;
			Point target;

			target = Evaluation::indirect_chipandchase_target(world).first;
			double chip_power = (target - world.ball().position()).len();
			AI::HL::STP::Action::catch_and_shoot_target(ca, world, player(), target, chip_power, true);
			yield(ca);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::chip_upfield(World world) {
	Tactic::Ptr p(new DefensiveChip(world));
	return p;
}
