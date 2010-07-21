#ifndef AI_WORLD_PLAYER_H
#define AI_WORLD_PLAYER_H

#include "ai/world/robot.h"
#include "robot_controller/robot_controller.h"
#include "uicomponents/annunciator.h"
#include "xbee/client/drive.h"
#include <ctime>
#include <map>
#include <typeinfo>

class AI;
class World;

/**
 * A player is a robot that can be driven.
 */
class Player : public Robot {
	public:
		/**
		 * A state block associated with a Player. AI classes that need to store
		 * permanent state on a per-player basis but that cannot do so inside
		 * themselves (by reason of being destroyed frequently) should subclass
		 * this class, add the necessary state information as fields in the
		 * subclass, and then use the Player::get_state and Player::set_state
		 * functions to retrieve and store the state block.
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
		 * \return the player's 64-bit address.
		 */
		uint64_t address() const;

		/**
		 * \return the maximum dribble distance.
		 */
		static const double MAX_DRIBBLE_DIST;

		/**
		 * Instructs the player to move.
		 * \param dest the destination point to move to
		 * \param ori the target origin to rotate to
		 */
		void move(const Point &dest, double ori);

		/**
		 * Sets the speed of the dribbler motor. If this is not invoked by the
		 * AI in a particular time tick, the dribbler will turn off.
		 * \param speed the speed to run at, from 0 to 1
		 */
		void dribble(double speed);

		/**
		 * \return the number of milliseconds until the chicker is ready to use.
		 */
		unsigned int chicker_ready_time() const;

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
		 * \return the number of consecutive times the robot's dribbler sense the ball.
		 * WARNING!!! This can be a false positive,
		 * especially if the dribbler is spinning up or down.
		 */
		int sense_ball() const {
			return sense_ball_;
		}

		/**
		 * \return The number of seconds for which the player has sensed the ball.
		 */
		double sense_ball_time() const;
	
		/**
		 * \return The number of seconds elapsed since the player has sensed the ball.
		 * May stop the AI from panicking if the player losses the ball temporarily.
		 */
		double last_sense_ball_time() const;
	
		/**
		 * \return The speed the dribbler would be spinning at given the
		 * current power level if it were spinning unloaded and if it had been
		 * given sufficient time to stabilize, in RPM
		 */
		unsigned int theory_dribbler_speed() const {
			return theory_dribble_rpm;
		}

		/**
		 * \return The speed the dribbler is running at, in RPM
		 */
		unsigned int dribbler_speed() const {
			return bot->dribbler_speed();
		}

		/**
		 * \return the distance this player has travelled while dribbling the
		 * ball, in metres (or \c 0.0 if the player is not dribbling now).
		 */
		double dribble_distance() const {
			return dribble_distance_;
		}

		/**
		 * \return Whether the dribbler is safe to use, i.e. won't stall the
		 * motor for too long.
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
		 * is associated with the given class
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
		 * If chicker takes this amount of time to recharge,
		 * then chicker is R.I.P.
		 */
		static const unsigned int CHICKER_FOREVER;

	private:
		XBeeDriveBot::Ptr bot;
		Point destination_;
		double target_orientation;
		RobotController2::Ptr controller;
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
		Annunciator::message not_moved_message, chick_when_not_ready_message;

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
		 * \param[in] bot the XBee robot being driven
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
		 * \param[in] bot the XBee robot being driven
		 */
		Player(const Glib::ustring &name, bool yellow, unsigned int pattern_index, XBeeDriveBot::Ptr bot);

		/**
		 * Drives one tick of time through the RobotController and to the XBee.
		 * \param scram whether or not to scram the robot
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

#endif

