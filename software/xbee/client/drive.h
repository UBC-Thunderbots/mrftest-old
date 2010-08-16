#ifndef XBEE_CLIENT_DRIVE_H
#define XBEE_CLIENT_DRIVE_H

#include "uicomponents/annunciator.h"
#include "util/byref.h"
#include "util/time.h"
#include "xbee/client/packet.h"
#include "xbee/shared/packettypes.h"
#include <cstddef>
#include <ctime>
#include <glibmm.h>
#include <stdint.h>

class XBeeLowLevel;

/**
 * Allows access to a robot in drive mode. Drive mode is used to drive the
 * robot, for example in the control process or the tester.
 */
class XBeeDriveBot : public ByRef, public sigc::trackable {
	public:
		/**
		 * A pointer to an XBeeDriveBot.
		 */
		typedef RefPtr<XBeeDriveBot> Ptr;

		/**
		 * The 64-bit address of this robot.
		 */
		const uint64_t address;

		/**
		 * Fired when the robot is alive and communicating.
		 */
		sigc::signal<void> signal_alive;

		/**
		 * Fired when the robot was communicating and has stopped.
		 */
		sigc::signal<void> signal_dead;

		/**
		 * Fired every time the robot sends feedback data.
		 */
		sigc::signal<void> signal_feedback;

		/**
		 * Fired if the claim request failed because another client already owns
		 * the robot. You should destroy the object.
		 */
		sigc::signal<void> signal_claim_failed_locked;

		/**
		 * Fired if the claim request failed because there were not enough radio
		 * resources available to configure the robot. You should destroy the
		 * object.
		 */
		sigc::signal<void> signal_claim_failed_resource;

		/**
		 * Creates a new XBeeDriveBot and begins attempting to claim the bot.
		 *
		 * \param[in] name the name of the robot.
		 *
		 * \param[in] address the 64-bit address of the robot.
		 *
		 * \param[in] ll the connection to the dæmon.
		 *
		 * \return the new robot connection.
		 */
		static Ptr create(const Glib::ustring &name, uint64_t address, XBeeLowLevel &ll);

		/**
		 * Returns whether or not the robot is communicating.
		 *
		 * \return \c true if the robot is communicating, or \c false if not.
		 */
		bool alive() const {
			return alive_;
		}

		/**
		 * Checks whether a drive motor is faulted.
		 *
		 * \param motor the index of the motor to query, from 0 to 3.
		 *
		 * \return \c true if the requested motor experienced a fault recently,
		 * or \c false if not.
		 */
		bool drive_faulted(unsigned int motor) const;

		/**
		 * Checks whether the dribbler motor is faulted.
		 *
		 * \return \c true if the dribbler experienced a fault recently, or \c
		 * false if not.
		 */
		bool dribbler_faulted() const;

		/**
		 * Checks the battery voltage.
		 *
		 * \return the voltage of the robot's battery, in millivolts.
		 */
		unsigned int battery_voltage() const;

		/**
		 * Checks the capacitor voltage.
		 *
		 * \return the voltage of the robot's capacitor, in millivolts.
		 */
		unsigned int capacitor_voltage() const;

		/**
		 * Checks the dribbler speed.
		 *
		 * \return the speed at which the dribbler motor is spinning, in
		 * revolutions per minute.
		 */
		unsigned int dribbler_speed() const;

		/**
		 * Returns the current estimated radio latency.
		 *
		 * \return the estimated latency.
		 */
		const timespec &latency() const {
			return latency_;
		}

		/**
		 * Returns the feedback interval.
		 *
		 * \return the amount of time between two consecutive feedback packets
		 * from this robot.
		 */
		const timespec &feedback_interval() const {
			return feedback_interval_;
		}

		/**
		 * Returns the interval between outbound run data packets.
		 *
		 * \return the amount of time between two consecutive run data packets
		 * sent to the robot team.
		 */
		const timespec &run_data_interval() const {
			return run_data_interval_;
		}

		/**
		 * Returns the outbound signal strength for packets sent to this robot.
		 *
		 * \return the outbound received signal strength in dBm (between 0 and
		 * -255).
		 */
		int outbound_rssi() const;

		/**
		 * Returns the inbound signal strength for packets received from this
		 * robot.
		 *
		 * \return the inbound received signal strength in dBm (between 0 and
		 * -255).
		 */
		int inbound_rssi() const;

		/**
		 * Returns the success rate of feedback packets solicited from this
		 * robot.
		 *
		 * \return the number of the last 16 packets that were delivered
		 * successfully.
		 */
		unsigned int success_rate() const {
			return success_rate_;
		}

		/**
		 * Checks whether the chicker is ready.
		 *
		 * \return \c true if the chicker is ready to fire, or \c false if it is
		 * disabled or still charging.
		 */
		bool chicker_ready() const;

		/**
		 * Checks whether the LT3751 is faulting.
		 *
		 * \return \c true if the LT3751 chicker charger chip is faulting, or \c
		 * false if not.
		 */
		bool lt3751_faulted() const;

		/**
		 * Checks whether the chicker timed out passing the low voltage
		 * threshold.
		 *
		 * \return \c true if the chicker charger failed to charge above the
		 * minimum threshold voltage within the allowed time period, or \c false
		 * if not.
		 */
		bool chicker_low_faulted() const;

		/**
		 * Checks whether the chicker went over its maximum voltage.
		 *
		 * \return \c true if the chicker charger charged above the maximum
		 * volatge, or \c false if not.
		 */
		bool chicker_high_faulted() const;

		/**
		 * Checks whether the chicker timed out charging.
		 *
		 * \return \c true if the chicker charger timed out waiting for the
		 * LT3751 to signal completion of the charge process, or \c false if
		 * not.
		 */
		bool chicker_timed_out() const;

		/**
		 * Prevents the robot from being scrammed by a timeout, without actually
		 * affecting the current drive parameters.
		 */
		void stamp();

		/**
		 * Applies dynamic braking to the drive motors.
		 */
		void drive_scram();

		/**
		 * Drives the four motors with distinct power levels independently. Each
		 * parameter is a power level between -1023 and +1023.
		 *
		 * \param[in] m1 the power level of the front-left motor.
		 *
		 * \param[in] m2 the power level of the back-left motor.
		 *
		 * \param[in] m3 the power level of the back-right motor.
		 *
		 * \param[in] m4 the power level of the front-right motor.
		 */
		void drive_direct(int m1, int m2, int m3, int m4);

		/**
		 * Drives the four motors through the control loops. Each parameter is a
		 * motor speed measured in quarters of a degree of motor shaft rotation
		 * per five milliseconds between -1023 and +1023.
		 *
		 * \param[in] m1 the speed of the front-left motor.
		 *
		 * \param[in] m2 the speed of the back-left motor.
		 *
		 * \param[in] m3 the speed of the back-right motor.
		 *
		 * \param[in] m4 the speed of the front-right motor.
		 */
		void drive_controlled(int m1, int m2, int m3, int m4);

		/**
		 * Sets the power level of the dribbler. The parameter is a power level
		 * between -1023 and +1023. A power level of zero applies dynamic
		 * braking to the dribbler motor.
		 *
		 * \param[in] p the power level.
		 */
		void dribble(int p);

		/**
		 * Enables or disables the chicker subsystem.
		 *
		 * \param[in] en \c true to enable the subsystem and begin charging, or
		 * \c false to disable the subsystem and shut off the charger.
		 */
		void enable_chicker(bool en);

		/**
		 * Fires the kicker.
		 *
		 * \param[in] width the width of the pulse to generate, in 32ms units.
		 */
		void kick(unsigned int width);

		/**
		 * Fires the chipper.
		 *
		 * \param[in] width the width of the pulse to generate, in 32ms units.
		 */
		void chip(unsigned int width);

	private:
		XBeeLowLevel &ll;
		bool alive_;
		XBeePacketTypes::SHM_FRAME *shm_frame;
		XBeePacketTypes::FEEDBACK_DATA feedback_;
		timespec latency_, feedback_timestamp_, feedback_interval_, run_data_interval_;
		uint8_t inbound_rssi_;
		unsigned int success_rate_;
		timespec low_battery_start_time, lt3751_fault_start_time;
		Annunciator::Message low_battery_message, lt3751_fault_message, chicker_low_fault_message, chicker_high_fault_message, chicker_charge_timeout_message;

		XBeeDriveBot(const Glib::ustring &name, uint64_t address, XBeeLowLevel &ll);
		~XBeeDriveBot();
		void on_meta(const void *, std::size_t);
		void clear_chick();
};

#endif

