#ifndef XBEE_CLIENT_DRIVE_H
#define XBEE_CLIENT_DRIVE_H

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
		static ptr create(uint64_t address, xbee_lowlevel &ll) {
			ptr p(new xbee_drive_bot(address, ll));
			return p;
		}

		//
		// Returns whether or not the robot is communicating.
		//
		bool alive() const {
			return alive_;
		}

		//
		// Returns the current feedback data.
		//
		const xbeepacket::FEEDBACK_DATA &feedback() const {
			return feedback_;
		}

		//
		// Returns the current estimated radio latency.
		//
		const timespec &latency() const {
			return latency_;
		}

		//
		// Returns the inbound received signal strength.
		//
		uint8_t inbound_rssi() const {
			return inbound_rssi_;
		}

		//
		// Returns the number of the last 64 packets that were delivered
		// successfully.
		//
		unsigned int success_rate() const {
			return success_rate_;
		}

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

		//
		// Fires the kicker.
		//
		void kick(unsigned int);

		//
		// Fires the chipper.
		//
		void chip(unsigned int);

	private:
		xbee_lowlevel &ll;
		bool alive_;
		xbeepacket::SHM_FRAME *shm_frame;
		xbeepacket::FEEDBACK_DATA feedback_;
		timespec latency_;
		uint8_t inbound_rssi_;
		unsigned int success_rate_;

		xbee_drive_bot(uint64_t address, xbee_lowlevel &ll);
		~xbee_drive_bot();
		void on_meta(const void *, std::size_t);
		void clear_chick();
};

#endif

