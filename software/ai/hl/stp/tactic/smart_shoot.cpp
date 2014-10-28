#include "ai/hl/stp/tactic/smart_shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/tactic/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "ai/hl/stp/action/pivot.h"
#include "geom/util.h"

#include <iostream>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Action;
using namespace AI::HL::W;

namespace {
	class SmartShoot final : public Tactic {
		public:
			explicit SmartShoot(World world, const Point target, double speed_ratio) : Tactic(world, true), target(target), speed_ratio(speed_ratio) {
			}

		private:
			Point target;
			double speed_ratio;
			bool done() const override {
				return player && player.autokick_fired();
			}

			Player select(const std::set<Player> &players) const override {
				return select_baller(world, players, player);
			}

			void execute() override {
				Point dest;
				Point player_to_ball = player.position() - world.ball().position();
				Point move_back_distance = Point(0.05, 0.05);
				Angle epsilon = Angle::of_degrees(4);
				Angle to_target = ((world.ball().position() - target).orientation()) - player_to_ball.orientation();
				Angle point_our_net_lower = Angle::of_degrees(210);
				Angle point_our_net_higher = Angle::of_degrees(150);

				Angle player_orientation = player.orientation();
				player.move(player.position(), to_target, Point());


				if ((player_orientation.angle_diff(to_target) < epsilon)) {
					if (obstacle(player, target) == true && chipperclear(player) == true) {
						std::cout << "Chip chip";
						player.autochip(speed_ratio);
						}
/*
 * 					TODO: Add code so that if enemy is too close for robot to chip, it moves back a bit to make room and then chips.
 */
					else if (obstacle(player, target) == true && chipperclear(player) == false) {

						//ASK TERENCE ABOUT ILLEGAL ZONES
						if ((player.position() - move_back_distance) < (world.field().friendly_goal()) && (player.position() - move_back_distance) > (world.field().enemy_goal())) {

							// dont back up if the player is pointing towards our net
							if ((player_orientation > point_our_net_lower) && (player_orientation < point_our_net_higher))
								player.move(Point(player.position() - move_back_distance), player.orientation(), Point());
						}
					}
					else {
						std::cout << "shoot shoot";
						player.autokick(speed_ratio);
					}
				}
			}


			/*
			 * Returns true if the robot has no enemies close enough to it to allow the robot to chip.
			 * clearDistance is double value subject to change.
			 */

			bool chipperclear(Player player) {
				double clearDistance = 0.18;

				for (const Robot i : world.enemy_team()) {
					Point displacement = i.position() - player.position();

					if (displacement.len() < clearDistance && player.orientation().angle_mod().to_radians() < M_PI / 2 && player.orientation().angle_mod().to_radians() > -M_PI / 2) {
						return false;
					}
				}

				return true;
			}


			/*
			 * If there are any robots (friendly or enemy) in the path of the robot to the target, it returns true.
			 * If there are not any robots in the path of the robot to the target, it returns false.
			 */

			bool obstacle(Player Passer, Point Destination) {
				double tolerance = Robot::MAX_RADIUS/2;
				Point rectangle[4];
				Point norm_passer = (Passer.position() - Destination).norm();
				//rectangle drawn by getting normal vector. then the 4 points are chosen by tolerance
				rectangle[0] = Passer.position() + (norm_passer * tolerance);
				rectangle[1] = Passer.position() - (norm_passer * tolerance);
				rectangle[2] = Destination + (norm_passer * tolerance);
				rectangle[3] = Destination - (norm_passer * tolerance);
				//check if any enemies are in the rectangle
				for (const Robot i : world.enemy_team()) {
					if (point_in_rectangle(i.position(), rectangle)) {
						return true;
					}
				}
				//check if any friendlies are in the rectangle
				for (const Player i : world.friendly_team()) {
					if (point_in_rectangle(i.position(), rectangle)) {
						return true;
					}
				}
				//return false if rectangle is clear of obstacles
				return false;
			}

			Glib::ustring description() const {
				return u8"smart shoot";
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::smart_shoot(World world, const Point target, double speed) {
	Tactic::Ptr p(new SmartShoot(world, target, speed));
	return p;
}
