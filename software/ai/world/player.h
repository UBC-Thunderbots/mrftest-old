#ifndef AI_WORLD_PLAYER_H
#define AI_WORLD_PLAYER_H

#include "ai/robot_controller/robot_controller.h"
#include "ai/world/robot.h"
#include "uicomponents/annunciator.h"
#include "xbee/client/drive.h"
#include <ctime>
#include <map>
#include <typeinfo>

namespace AI {
	class AI;
	class World;

	/**
	 * A player is a robot that can be driven.
	 */
	class Player : public Robot {
		public:
			/**
			 * A state block associated with a Player. %AI classes that need to
			 * store permanent state on a per-player basis but that cannot do so
			 * inside themselves (by reason of being destroyed frequently) should
			 * subclass this class, add the necessary state information as fields in
			 * the subclass, and then use the Player::get_state and
			 * Player::set_state functions to retrieve and store the state block.
			 */
			class State : public ByRef {
				public:
					/**
					 * A pointer to a State block.
					 */
					typedef RefPtr<State> Ptr;

				protected:
					/**
					 * Constructs a new State block.
					 */
					State();

					/**
					 * Destroys the State block.
					 */
					virtual ~State();
			};

			/**
			 * A pointer to a Player.
			 */
			typedef RefPtr<Player> Ptr;

			/**
			 * Gets the player's 64-bit address.
			 *
			 * \return the player's 64-bit address.
			 */
			uint64_t address() const;

			/**
			 * The maximum dribble distance.
			 */
			static const double MAX_DRIBBLE_DIST;

			/**
			 * Instructs the player to move.
			 *
			 * \param[in] dest the destination point to move to.
			 *
			 * \param[in] ori the target origin to rotate to.
			 */
			void move(const Point &dest, double ori);

			/**
			 * Sets the speed of the dribbler motor. If this is not invoked by the
			 * %AI in a particular time tick, the dribbler will turn off.
			 *
			 * \param[in] speed the speed to run at, from 0 to 1.
			 */
			void dribble(double speed);

			/**
			 * Gets the delay until the chicker is ready.
			 *
			 * \return the number of milliseconds until the chicker is ready to use.
			 */
			unsigned int chicker_ready_time() const;

			/**
			 * Fires the kicker.
			 *
			 * \param[in] power the power to kick at, from 0 to 1.
			 */
			void kick(double power);

			/**
			 * Fires the chipper.
			 *
			 * \param[in] power the power to chip at, from 0 to 1.
			 */
			void chip(double power);

			/**
			 * Gets the number of times the dribbler motor has sensed load
			 * proportional to having the ball. This can have false positives,
			 * especially if the dribbler is spinning up or down.
			 *
			 * \return the number of consecutive times the robot's dribbler sensed
			 * the ball.
			 */
			int sense_ball() const {
				return sense_ball_;
			}

			/**
			 * Gets the number of seconds the dribbler motor has sensed load
			 * proportional to having the ball. This can have false positives,
			 * especially if the dribbler is spinning up or down.
			 *
			 * \return the number of seconds for which the robot's dribbler sensed
			 * the ball.
			 */
			double sense_ball_time() const;

			/**
			 * Gets the number of seconds since the player stopped sending the ball
			 * on its dribbler. This can be used to detect a momentary loss of
			 * sensing as compared to a complete genuine loss of ball.
			 *
			 * \return the number of seconds elapsed since the player last sensed
			 * the ball.
			 */
			double last_sense_ball_time() const;

			/**
			 * Gets the speed the dribbler would be spinning at given the current
			 * power level if it were unloaded and had finished spinning up.
			 *
			 * \return the theoretical dribbler speed, in RPM.
			 */
			unsigned int theory_dribbler_speed() const {
				return theory_dribble_rpm;
			}

			/**
			 * Gets the current speed the dribbler is spinning.
			 *
			 * \return the dribbler speed, in RPM.
			 */
			unsigned int dribbler_speed() const {
				return bot->dribbler_speed();
			}

			/**
			 * Gets the distance the player has travelled while dribbling the ball.
			 *
			 * \return the distance in metres.
			 */
			double dribble_distance() const {
				return dribble_distance_;
			}

			/**
			 * Checks whether the dribbler is currently safe to use (i.e. isn't too
			 * hot from stallling a lot).
			 *
			 * \return \c true if the dribbler can be used, or \c false if not.
			 */
			bool dribbler_safe() const;

			/**
			 * Fetches the State block previously stored by some class.
			 *
			 * \param[in] tid the type of the class whose State should be fetched;
			 * you probably want to pass \c typeid(*this) here.
			 *
			 * \return the corresponding State block (which can be cast to a derived
			 * type with RefPtr::cast_dynamic), or a null pointer if no State
			 * is associated with the given class.
			 */
			State::Ptr get_state(const std::type_info &tid) const;

			/**
			 * Stores a State block for a class. Any previously-stored State block
			 * for the same class will be dereferenced and, if not pointed to by any
			 * other pointers, destroyed.
			 *
			 * \param[in] tid the type of the class whose State should be stored;
			 * you probably want to pass \c typeid(*this) here.
			 *
			 * \param[in] state the new State to store (which can be a null pointer
			 * to remove the State).
			 */
			void set_state(const std::type_info &tid, State::Ptr state);

			/**
			 * The robot's name.
			 */
			const Glib::ustring name;

			/**
			 * The value returned by chicker_ready_time() const if the chicker is
			 * probably never going to be ready.
			 */
			static const unsigned int CHICKER_FOREVER;

		private:
			XBeeDriveBot::Ptr bot;
			Point destination_;
			double target_orientation;
			RobotController::RobotController2::Ptr controller;
			bool moved;
			int new_dribble_power;
			int old_dribble_power;
			int sense_ball_;
			bool dribble_stall;
			unsigned int theory_dribble_rpm;
			timespec sense_ball_start, sense_ball_end, stall_start, recover_time_start, chicker_last_fire_time;
			double dribble_distance_;
			Point last_dribble_position;
			std::map<const std::type_info *, State::Ptr, bool (*)(const std::type_info *, const std::type_info *)> state_store;
			Annunciator::Message not_moved_message, chick_when_not_ready_message;

			/**
			 * Constructs a new Player object.
			 *
			 * \param[in] name the robot's name.
			 *
			 * \param[in] yellow \c true if the new robot is yellow, or \c false if
			 * it is blue.
			 *
			 * \param[in] pattern_index the index of the vision pattern associated
			 * with the player.
			 *
			 * \param[in] bot the XBee robot being driven.
			 *
			 * \return the new object.
			 */
			static Ptr create(const Glib::ustring &name, bool yellow, unsigned int pattern_index, XBeeDriveBot::Ptr bot);

			/**
			 * Constructs a new Player object.
			 *
			 * \param[in] name the robot's name.
			 *
			 * \param[in] yellow \c true if the new robot is yellow, or \c false if
			 * it is blue.
			 *
			 * \param[in] pattern_index the index of the vision pattern associated
			 * with the player.
			 *
			 * \param[in] bot the XBee robot being driven.
			 */
			Player(const Glib::ustring &name, bool yellow, unsigned int pattern_index, XBeeDriveBot::Ptr bot);

			/**
			 * Drives one tick of time through the RobotController and to the XBee.
			 *
			 * \param[in] scram whether or not to scram the robot.
			 */
			void tick(bool scram);

			Visualizable::RobotColour visualizer_colour() const {
				return Visualizable::RobotColour(0.0, 1.0, 0.0);
			}

			bool has_destination() const {
				return true;
			}

			Point destination() const {
				return destination_;
			}

			/**
			 * Process that makes sure that the dribble motor is not stalled for too
			 * long.
			 */
			void dribbler_safety();

			void on_feedback();

			friend class AI;
			friend class World;
	};
}

#endif

