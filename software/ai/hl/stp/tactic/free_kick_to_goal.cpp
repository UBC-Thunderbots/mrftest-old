#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/indirect_chip.h"
#include "ai/hl/stp/tactic/free_kick_to_goal.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include "util/param.h"
#include "geom/util.h"
#include "geom/angle.h"

using namespace std;
namespace Primitives = AI::BE::Primitives;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;
using namespace Geom;


namespace {
	class FreeKickToGoal final : public Tactic {
		public:
			explicit FreeKickToGoal(World world) : Tactic(world) {
			}

		private:
			void execute(caller_t& ca) override;
			Player select(const std::set<Player> &players) const override;

			Glib::ustring description() const override {
				return u8"free-kick-to-goal";
			}
	};

	Player FreeKickToGoal::select(const std::set<Player> &players) const {
		return select_baller(world, players, player());
	}

	void FreeKickToGoal::execute(caller_t& ca) {
		while (true) {
			player().move_move(world.ball().position(), (world.ball().position() - player().position()).orientation());

			Point gp1 = world.field().enemy_goal_boundary().first;
			Point gp2 = world.field().enemy_goal_boundary().second;

			Angle total_angle = vertex_angle(gp1 , world.ball().position(), gp2).angle_mod().abs();

			//player can see 15% of the net. Will shoot on goal
			if(Evaluation::get_best_shot_pair(world, player()).second / total_angle > 0.15){
				Action::shoot_target(ca, world, player(), Evaluation::get_best_shot(world, player()));
				//LOGF_INFO(u8"TARGET: %1, POWER: %2, TYPE: %3", "net", "max", "shot on goal");
			}
			//player can see at least 15% of the net (not including close enemies). Will chip at net
			else if(Evaluation::indirect_chip_target(world, player()).second / total_angle > 0.15) {
				Point target = Evaluation::indirect_chip_target(world, player()).first;
				double chip_power = (target - world.ball().position()).len();
				Action::shoot_target(ca, world, player(), target, chip_power, true);
				//LOGF_INFO(u8"TARGET: %1, POWER: %2, TYPE: %3", target, chip_power, "chip at net");
			}
			// If there is enough space to chip and chase, try chip and chase
			else if(Evaluation::indirect_chipandchase_target(world).second == true) {
				Point target = Evaluation::indirect_chipandchase_target(world).first;
				double chip_power = (target - world.ball().position()).len();
				Action::shoot_target(ca, world, player(), target, chip_power, true);
				//LOGF_INFO(u8"TARGET: %1, POWER: %2, TYPE: %3", target, chip_power, "chip and chase");
			}
			// Try to ricochet the ball off an enemy to get another kick
			else {
				Point target = Evaluation::deflect_off_enemy_target(world);
				Action::shoot_target(ca, world, player(), target);
				//LOGF_INFO(u8"TARGET: %1, POWER: %2, TYPE: %3", target, "MAX", "shank shot");
			}
			yield(ca);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::free_kick_to_goal(World world) {
	Tactic::Ptr p(new FreeKickToGoal(world));
	return p;
}
