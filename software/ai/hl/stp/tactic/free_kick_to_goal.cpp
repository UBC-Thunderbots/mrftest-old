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
DoubleParam chip_power(u8"chipping power level", "AI/HL/STP/Tactic/free_kick_to_goal", 1, 0.0, 8.0);
	class FreeKickToGoal final : public Tactic { public:
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
			Point gp1 = world.field().enemy_goal_boundary().first;
			Point gp2 = world.field().enemy_goal_boundary().second;

			Angle total_angle = vertex_angle(gp1 , world.ball().position(), gp2).angle_mod().abs();


			//if the ball is really close to the far end of the field, run the corner-kick play, if not, run normal free kick play
			if ( world.ball().position().x > ( world.field().enemy_goal().x - world.field().defense_area_radius() ) ) {

				Point targetDefensiveCrease = world.field().enemy_goal();

				targetDefensiveCrease.x = targetDefensiveCrease.x - world.field().defense_area_radius() - 0.1; //set the target to be on the "D" of the goal crease

				//check to see which side of the net to shoot on
//				if (world.ball().position().y > 0) {
//				targetDefensiveCrease.y = targetDefensiveCrease.y + 0.5;	//TODO: change this to change side depending on which side he robot is on
//				}
//				else {
//					targetDefensiveCrease.y = targetDefensiveCrease.y - 0.5;
//				}
//
//				ALTERNATEIVE LOCATION
				if (world.ball().position().y > 0) {
					targetDefensiveCrease.y = targetDefensiveCrease.y - 0.5;	//TODO: change this to change side depending on which side he robot is on
				}
				else {
					targetDefensiveCrease.y = targetDefensiveCrease.y + 0.5;
				}


				Action::shoot_target(ca, world, player(), targetDefensiveCrease, chip_power, true );

		}  else if(Evaluation::get_best_shot_pair(world, player()).second / total_angle > 0.15) { //player can see 15% of the net. Will shoot on goal
				Action::shoot_target(ca, world, player(), Evaluation::get_best_shot(world, player()));
				//LOGF_INFO(u8"TARGET: %1, POWER: %2, TYPE: %3", "net", "max", "shot on goal");
			}
			//player can see at least 15% of the net (not including close enemies). Will chip at net
			else /*(Evaluation::indirect_chip_target(world, player()).second / total_angle > 0.15)*/ {
				Point target = Evaluation::indirect_chip_target(world, player()).first;
				double chip_power = (target - world.ball().position()).len() * 0.5;
				Action::shoot_target(ca, world, player(), target, chip_power, true);
				//LOGF_INFO(u8"TARGET: %1, POWER: %2, TYPE: %3", target, chip_power, "chip at net");
			}

			# warning uncomment this later. We should have slightly different logic for free kick vs kickoff

//			// If there is enough space to chip and chase, try chip and chase
//			else if(Evaluation::indirect_chipandchase_target(world).second == true) {
//				Point target = Evaluation::indirect_chipandchase_target(world).first;
//				# warning the constant to reduce chip dist is a quick robocup hack. Should rather change evaluation of where to chip or the calibration
//				double chip_power = (target - world.ball().position()).len() * 0.8;
//				Action::shoot_target(ca, world, player(), target, chip_power, true);
//				//LOGF_INFO(u8"TARGET: %1, POWER: %2, TYPE: %3", target, chip_power, "chip and chase");
//			}
//			// Try to ricochet the ball off an enemy to get another kick
//			else {
//				Point target = Evaluation::deflect_off_enemy_target(world);
//				Action::shoot_target(ca, world, player(), target);
//				//LOGF_INFO(u8"TARGET: %1, POWER: %2, TYPE: %3", target, "MAX", "shank shot");
//			}
			yield(ca);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::free_kick_to_goal(World world) {
	Tactic::Ptr p(new FreeKickToGoal(world));
	return p;
}
