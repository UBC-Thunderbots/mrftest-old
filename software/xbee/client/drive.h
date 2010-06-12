#ifndef XBEE_CLIENT_DRIVE_H
#define XBEE_CLIENT_DRIVE_H

#include "uicomponents/annunciator.h"
#include "util/byref.h"
#include "xbee/client/packet.h"
#include "xbee/shared/packettypes.h"
#include <cstddef>
#include <ctime>
#include <glibmm.h>
#include <stdint.h>

class xbee_lowlevel;

//
// Allows access to a robot in drive mode. Drive mode is used to drive the
// robot, for example in the control process or the tester.
//
class xbee_drive_bot : public byref, public sigc::trackable {
	public:
		//
		// A pointer to an xbee_drive_bot.
		//
		typedef Glib::RefPtr<xbee_drive_bot> ptr;

		//
		// The 64-bit address of this robot.
		//
		const uint64_t address;

		//
		// Fired when the robot is alive and communicating.
		//
		sigc::signal<void> signal_alive;

		//
		// Fired when the robot was communicating and has stopped.
		//
		sigc::signal<void> signal_dead;

		//
		// Fired every time the robot sends feedback data.
		//
		sigc::signal<void> signal_feedback;

		//
		// Fired if the claim request failed because another client already owns
		// the robot. You should destroy the object.
		//
		sigc::signal<void> signal_claim_failed_locked;

		//
		// Fired if the claim request failed because there were not enough radio
		// resources available to configure the robot. You should destroy the
		// object.
		//
		sigc::signal<void> signal_claim_failed_resource;

		//
		// Creates a new xbee_drive_bot and begins attempting to claim the bot.
		//
		static ptr create(const Glib::ustring &name, uint64_t address, xbee_lowlevel &ll);

		//
		// Returns whether or not the robot is communicating.
		//
		bool alive() const {
			return alive_;
		}

		/**
		 * \param motor the index of the motor to query, from 0 to 3.
		 *
		 * \return true if the requested motor experienced a fault recently, or
		 * false if not
		 */
		bool drive_faulted(unsigned int motor) const;

		/**
		 * \return true if the dribbler experienced a fault recently, or false
		 * if not
		 */
		bool dribbler_faulted() const;

		/**
		 * \return The voltage of the robot's battery, in millivolts
		 */
		unsigned int battery_voltage() const;

		/**
		 * \return The speed at which the dribbler motor is spinning, in
		 * revolutions per minute.
		 */
		unsigned int dribbler_speed() const;

		//
		// Returns the current estimated radio latency.
		//
		const timespec &latency() const {
			return latency_;
		}

		/**
		 * \return the amount of time between two consecutive feedback packets
		 * from this robot.
		 */
		const timespec &feedback_interval() const {
			return feedback_interval_;
		}

		/**
		 * \return the amount of time between two consecutive run data packets
		 * sent to the robot team.
		 */
		const timespec &run_data_interval() const {
			return run_data_interval_;
		}

		/**
		 * \return The outbound received signal strength in dBm (between 0 and
		 * -255)
		 */
		int outbound_rssi() const;

		/**
		 * \return The inbound received signal strength in dBm (between 0 and
		 * -255)
		 */
		int inbound_rssi() const;

		/**
		 * \return the number of the last 16 packets that were delivered
		 * successfully.
		 */
		unsigned int success_rate() const {
			return success_rate_;
		}

		/**
		 * \return true if the chicker is ready to fire, or false if it is
		 * disabled or still charging
		 */
		bool chicker_ready() const;

		/**
		 * \return true if the chicker charger experienced a fault recently, or
		 * false if not
		 */
		bool chicker_faulted() const;

		//
		// Prevents the robot from being scrammed by a timeout, without actually
		// affecting the current drive parameters.
		//
		void stamp();

		//
		// Applies dynamic braking to the drive motors.
		//
		void drive_scram();

		//
		// Drives the four motors with distinct power levels independently. Each
		// parameter is a power level between -1023 and +1023.
		//
		void drive_direct(int, int, int, int);

		//
		// Drives the four motors through the control loops. Each parameter is a
		// motor speed measured in quarters of a degree of motor shaft rotation
		// per five milliseconds between -1023 and +1023.
		//
		void drive_controlled(int, int, int, int);

		//
		// Sets the power level of the dribbler. The parameter is a power level
		// between -1023 and +1023. A power level of zero applies dynamic
		// braking to the dribbler motor.
		//
		void dribble(int);

		//
		// Enables or disables the chicker subsystem.
		//
		void enable_chicker(bool);

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
		xbee_lowlevel &ll;
		bool alive_;
		xbeepacket::SHM_FRAME *shm_frame;
		xbeepacket::FEEDBACK_DATA feedback_;
		timespec latency_, feedback_timestamp_, feedback_interval_, run_data_interval_;
		uint8_t inbound_rssi_;
		unsigned int success_rate_;
		timespec low_battery_start_time, chicker_fault_start_time;
		annunciator::message low_battery_message, chicker_fault_message;

		xbee_drive_bot(const Glib::ustring &name, uint64_t address, xbee_lowlevel &ll);
		~xbee_drive_bot();
		void on_meta(const void *, std::size_t);
		void clear_chick();
};

#endif

