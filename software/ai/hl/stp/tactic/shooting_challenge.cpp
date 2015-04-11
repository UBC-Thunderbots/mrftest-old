#include "ai/hl/stp/tactic/shooting_challenge.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/tactic/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "ai/hl/stp/action/pivot.h"
#include "geom/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Action;
using namespace AI::HL::W;
using namespace Geom;

namespace {
	class ShootingChallenge final : public Tactic {
		public:
			explicit ShootingChallenge(World world, double speed_ratio) : Tactic(world, true), goal_point(world.field().enemy_goal()), shooter(player.position() - world.ball().position()), to_target((world.ball().position() - goal_point).orientation() - shooter.orientation()), speed_ratio(speed_ratio) {
			}

		private:
			Point goal_point;
			Point shooter;
			Angle to_target;
			double speed_ratio;
			bool done() const override {
				return player && player.autokick_fired();
			}

			Player select(const std::set<Player> &players) const override {
				return select_baller(world, players, player);
			}

			void execute() override {
				AI::HL::STP::Action::shoot_goal(world, player, true);

				//					Angle to_target = ((world.ball().position() - goal_point).orientation()) - shooter.orientation();
//					player.move(player.position(), to_target, Point());
//						if (player.has_ball() == true && obstacle(player, goal_point) == false) {
//							player.autokick(speed_ratio);
//	/					}
			}

			bool obstacle(Player passer, Point destination) {
				double tolerance = Robot::MAX_RADIUS/2;
				Seg path = Seg(passer.position(), destination);

				for (const Player i : world.friendly_team()) {
					// ignore ourself
					if (i == passer) continue;

					if (dist(i.position(), path) < tolerance) return true;
				}

				for (const Robot i : world.enemy_team()) {
					if (dist(i.position(), path) < tolerance) return true;
				}

				return false;
			}

			Glib::ustring description() const {
				return u8"Shooting Challenge";
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::shooting_challenge(World world, double speed) {
	Tactic::Ptr p(new ShootingChallenge(world, speed));
	return p;
}

