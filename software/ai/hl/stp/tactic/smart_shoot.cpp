#include "ai/hl/stp/tactic/smart_shoot.h"
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
	class SmartShoot : public Tactic {
		public:
			SmartShoot(World world, const Point target, double speed_ratio) : Tactic(world, true), target(target), speed_ratio(speed_ratio) {
			}

		private:
			Point target;
			double speed_ratio;
			bool done() const {
				return player && player->autokick_fired();
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute() {
				Point dest;
				Point player_to_ball = player->position() - world.ball().position();
//				Point move_back_distance = Point(0.05, 0.05);
				Angle epsilon = Angle::of_degrees(4);
				Angle to_target = ((world.ball().position() - target).orientation()) - player_to_ball.orientation();

				Angle player_orientation = player->orientation();
				player->move(player->position(), to_target, Point());

				if ((player_orientation.angle_diff(to_target) < epsilon)) {
					if (obstacle(player, target) == true && chipperclear(player) == true) {
						player->autochip(speed_ratio);
						}
/*
 * 					TODO: Add code so that if enemy is too close for robot to chip, it moves back a bit to make room and then chips.
 */
//					else if (obstacle(player, target) == true && chipperclear(player) == false) {
//
//						//ASK TERENCE ABOUT ILLEGAL ZONES
//						if ((player->position() - move_back_distance) < (world.field().friendly_goal()) && (player->position() - move_back_distance) > (world.field().enemy_goal())) {
//							player->move(Point(player->position() - move_back_distance), player->orientation(), Point());
//						}
//					}
					else {
						player->autokick(speed_ratio);
					}
				}
			}


			/*
			 * Returns true if the robot has no enemies close enough to it to allow the robot to chip.
			 * clearDistance is double value subject to change.
			 */

			bool chipperclear(Player::Ptr Player) {
				std::size_t size_enemy = world.enemy_team().size();

				double clearDistance = 0.18;

				for (size_t i = 0; i < size_enemy; i++) {
					Point displacement = world.enemy_team().get(i)->position() - Player->position();

					if (displacement.len() < clearDistance)
						return false;
				}

				return true;
				}


			/*
			 * If there are any robots (friendly or enemy) in the path of the robot to the target, it returns true.
			 * If there are not any robots in the path of the robot to the target, it returns false.
			 */

			bool obstacle(Player::Ptr Passer, Point Destination) {
				std::size_t size_enemy = world.enemy_team().size();
				std::size_t size_friendly = world.friendly_team().size();
				double tolerance = Robot::MAX_RADIUS/2;
				Point rectangle[4];
				Point norm_passer = (Passer->position() - Destination).norm();
				//rectangle drawn by getting normal vector. then the 4 points are chosen by tolerance
				rectangle[0] = Passer->position() + (norm_passer * tolerance);
				rectangle[1] = Passer->position() - (norm_passer * tolerance);
				rectangle[2] = Destination + (norm_passer * tolerance);
				rectangle[3] = Destination - (norm_passer * tolerance);
				//check if any enemies are in the rectangle
				for (std::size_t i = 0; i < size_enemy; i++) {

					if (point_in_rectangle(world.enemy_team().get(i)->position(), rectangle) == true)
						return true;
					}
				//check if any friendlies are in the rectangle
				for (std::size_t i = 0; i < size_friendly; i++) {

					if (point_in_rectangle(world.friendly_team().get(i)->position(), rectangle) == true)
						return true;
				}
				//return false if rectangle is clear of obstacles
				return false;
			}

			Glib::ustring description() const {
				return "smart shoot";
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::smart_shoot(World world, const Point target, double speed) {
	Tactic::Ptr p(new SmartShoot(world, target, speed));
	return p;
}

