#include "ai/hl/stp/tactic/shooting_challenge.h"
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

namespace {
	class ShootingChallenge : public Tactic {
		public:
			ShootingChallenge(World world, double speed_ratio) : Tactic(world, true), goal_point(world.field().enemy_goal()), player_to_ball(player.position() - world.ball().position()), to_target((world.ball().position() - goal_point).orientation() - player_to_ball.orientation()), speed_ratio(speed_ratio) {
			}

		private:
			Point goal_point;
			Point player_to_ball;
			Angle to_target;
			double speed_ratio;
			bool done() const {
				return player && player.autokick_fired();
			}

			Player select(const std::set<Player> &players) const {
				return select_baller(world, players, player);
			}

			void execute() {
					Angle to_target = ((world.ball().position() - goal_point).orientation()) - player_to_ball.orientation();
					player.move(player.position(), to_target, Point());
						if (player.has_ball() == true && obstacle(player, goal_point) == false) {
							player.autokick(speed_ratio);
						}
					}

			bool obstacle(Player Passer, Point Destination) {
					std::size_t size_enemy = world.enemy_team().size();
					std::size_t size_friendly = world.friendly_team().size();
					double tolerance = Robot::MAX_RADIUS/2;
					Point rectangle[4];
					Point norm_passer = (Passer.position() - Destination).norm();
					//rectangle drawn by getting normal vector. then the 4 points are chosen by tolerance
					rectangle[0] = Passer.position() + (norm_passer * tolerance) ;
					rectangle[1] = Passer.position() - (norm_passer * tolerance);
					rectangle[2] = Destination + (norm_passer * tolerance);
					rectangle[3] = Destination - (norm_passer * tolerance);

					//check if any enemies are in the rectangle
					for (std::size_t i = 0; i < size_enemy; i++) {

						if (point_in_rectangle(world.enemy_team().get(i).position(), rectangle) == true)
							return true;
						}
					//check if any friendlies are in the rectangle
					for (std::size_t i = 0; i < size_friendly; i++) {

						if (point_in_rectangle(world.friendly_team().get(i).position(), rectangle) == true)
							return true;
					}
					//return false if rectangle is clear of obstacles
					return false;
				}

			Glib::ustring description() const {
			return "Shooting Challenge";
			}

	};
}

Tactic::Ptr AI::HL::STP::Tactic::shooting_challenge(World world, double speed) {
	Tactic::Ptr p(new ShootingChallenge(world, speed));
	return p;
}

