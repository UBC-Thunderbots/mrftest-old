#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"

#include <cassert>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;

namespace {
	Point old_des;

	class PenaltyGoalie final : public Tactic {
		public:
			explicit PenaltyGoalie(World world);

		private:
			bool goto_target1;
			bool done() const override;
			void execute() override;
//better implementation of chip power required. perhaps make a namespace variable for power?
			double power;
			Player select(const std::set<Player> &) const override {
				assert(false);
			}
			Glib::ustring description() const override {
				return "penalty-goalie";
			}
	};

	PenaltyGoalie::PenaltyGoalie(World world) : Tactic(world, true), goto_target1(false), power(0.6) {
		old_des = Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, -0.8 * Robot::MAX_RADIUS);
	}

	bool PenaltyGoalie::done() const {
		// it's never done!
		return false;
	}

	void PenaltyGoalie::execute() {
		if (random_penalty_goalie) {
			if ((player.position() - old_des).len() < AI::HL::Util::POS_CLOSE) {
				double ran = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
				old_des = Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, ran * 0.2 - (1 - ran) * 0.2);
			}
			// just orient towards the "front"
			AI::HL::STP::Action::move(player, Angle::zero(), old_des);
			player.type(AI::Flags::MoveType::RAM_BALL);
			player.prio(AI::Flags::MovePrio::HIGH);
		} else {
			const Point p1(world.field().friendly_goal().x + Robot::MAX_RADIUS/2, -0.2);
			const Point p2(world.field().friendly_goal().x + Robot::MAX_RADIUS/2, 0.2);
			if ((player.position() - p1).len() < AI::HL::Util::POS_CLOSE) {
				goto_target1 = false;
			} else if ((player.position() - p2).len() < AI::HL::Util::POS_CLOSE) {
				goto_target1 = true;
			}

			Point target;
			if (goto_target1) {
				target = p1;
			} else {
				target = p2;
			}
			// just orient towards the "front"
			AI::HL::STP::Action::move(player, Angle::zero(), target);
			player.type(AI::Flags::MoveType::RAM_BALL);
			player.prio(AI::Flags::MovePrio::HIGH);
		}

		if (player.has_ball()) {
			AI::HL::STP::Action::goalie_chip_target(world, player, world.field().enemy_goal());
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_goalie(World world) {
	Tactic::Ptr p(new PenaltyGoalie(world));
	return p;
}

