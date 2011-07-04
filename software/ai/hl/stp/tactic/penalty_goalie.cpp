#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"

#include <cassert>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;

namespace {
	Point old_des;
	
	class PenaltyGoalie : public Tactic {
		public:
			PenaltyGoalie(const World &world);

		private:
			bool goto_target1;
			bool done() const;
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
			std::string description() const {
				return "penalty-goalie";
			}
	};

	PenaltyGoalie::PenaltyGoalie(const World &world) : Tactic(world, true) {
		old_des = Point(world.field().friendly_goal().x, -0.8 * Robot::MAX_RADIUS);
	}

	bool PenaltyGoalie::done() const {
		// it's never done!
		return false;
	}

	void PenaltyGoalie::execute() {
		if (random_penalty_goalie) {
			if ((player->position() - old_des).len() < AI::HL::Util::POS_CLOSE) {
				double ran = (static_cast<double> (std::rand()) / static_cast<double> (RAND_MAX));
				old_des = Point(world.field().friendly_goal().x, ran * 0.2 - (1 - ran) * 0.2);
			}
			// just orient towards the "front"
			player->move(old_des, 0, Point());
			player->type(AI::Flags::MoveType::NORMAL);
			player->prio(AI::Flags::MovePrio::HIGH);
		} else {
			const Point p1(world.field().friendly_goal().x, -0.2);
			const Point p2(world.field().friendly_goal().x, 0.2);
			if ((player->position() - p1).len() < AI::HL::Util::POS_CLOSE) {
				goto_target1 = false;
			} else if ((player->position() - p2).len() < AI::HL::Util::POS_CLOSE) {
				goto_target1 = true;
			}

			Point target;
			if (goto_target1) {
				target = p1;
			} else {
				target = p2;
			}

			// just orient towards the "front"
			player->move(target, 0, Point());
			player->type(AI::Flags::MoveType::NORMAL);
			player->prio(AI::Flags::MovePrio::HIGH);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_goalie(const World &world) {
	const Tactic::Ptr p(new PenaltyGoalie(world));
	return p;
}

