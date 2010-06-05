#ifndef AI_WORLD_PLAYER_H
#define AI_WORLD_PLAYER_H

#include "ai/world/robot.h"
#include "robot_controller/robot_controller.h"
#include "xbee/client/drive.h"

class ai;
class world;

/**
 * A player is a robot that can be driven.
 */
class player : public robot {
	public:
		/**
		 * A pointer to a player.
		 */
		typedef Glib::RefPtr<player> ptr;

		/**
		 * Instructs the player to move.
		 * \param dest the destination point to move to
		 * \param ori the target origin to rotate to
		 */
		void move(const point &dest, double ori);

		/**
		 * Sets the speed of the dribbler motor. If this is not invoked by the
		 * AI in a particular time tick, the dribbler will turn off.
		 * \param speed the speed to run at, from 0 to 1
		 */
		void dribble(double speed);

		/**
		 * Fires the kicker.
		 * \param power the power to kick at, from 0 to 1
		 */
		void kick(double power);

		/**
		 * Fires the chipper.
		 * \param power the power to chip at, from 0 to 1
		 */
		void chip(double power);

		/**
		 * \return true if this player has control of the ball, or false if not
		 */
		bool has_ball() const {
			return has_ball_;
		}

		/**
		 * \return amount of ticks the player has the ball.
		 */
		int has_ball_count() const {
			return has_ball_count_;
		}

		/**
		*returns what the dribbler should be spinning at with no load on the 
		*dribbler motor
		*/
		unsigned int theory_dribbler_speed(){
			return theory_dribble_rpm;
		}
		/**
		*returns the speed that the dribbler is running at
		*/
		unsigned int dribbler_speed(){
			return bot->dribbler_speed();
		}

	private:
		xbee_drive_bot::ptr bot;
		point destination_;
		double target_orientation;
		robot_controller2::ptr controller;
		bool moved;
		int new_dribble_power;
		int old_dribble_power;
		bool has_ball_;
		int has_ball_count_;
		unsigned int theory_dribble_rpm;
		/**
		 * Constructs a new player object.
		 * \param bot the XBee robot being driven
		 * \return the new object
		 */
		static ptr create(bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot);

		/**
		 * Constructs a new player object.
		 * \param bot the XBee robot being driven
		 */
		player(bool yellow, unsigned int pattern_index, xbee_drive_bot::ptr bot);

		/**
		 * Drives one tick of time through the robot_controller and to the XBee.
		 * \param scram whether or not to scram the robot
		 */
		void tick(bool scram);

		visualizable::colour visualizer_colour() const {
			return visualizable::colour(0.0, 1.0, 0.0);
		}

		bool has_destination() const {
			return true;
		}

		point destination() const {
			return destination_;
		}

		void on_feedback();

		friend class ai;
		friend class world;
};

#endif

