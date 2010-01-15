#ifndef XBEE_BOT_H
#define XBEE_BOT_H

#include "util/byref.h"
#include "xbee/packettypes.h"
#include "xbee/xbee.h"
#include <cassert>
#include <stdint.h>

//
// Allows sending regular operational packets to a bot.
//
class radio_bot : public byref {
	public:
		//
		// A pointer to a radio_bot.
		//
		typedef Glib::RefPtr<radio_bot> ptr;

		//
		// Constructs a new radio_bot. The bot will not be sent packets initially.
		//
		radio_bot(xbee &modem, uint64_t address);

		//
		// Destroys a radio_bot.
		//
		~radio_bot();

		//
		// Starts sending packets to the bot.
		//
		void start();

		//
		// Stops sending packets to the bot.
		//
		void stop();

		//
		// Applies regenerative braking to the drive motors.
		//
		void drive_scram();

		//
		// Drives the four motors with direct power levels independently. Each
		// parameter is a power level between -1023 and 1023.
		//
		void drive_direct(int16_t m1, int16_t m2, int16_t m3, int16_t m4);

		//
		// Drives the four motors through the control loops. Each parameter is a
		// motor speed measured in quarters of a degree of motor shaft rotation
		// per five milliseconds between -1023 and 1023.
		//
		void drive_controlled(int16_t m1, int16_t m2, int16_t m3, int16_t m4);

		//
		// Applies regenerative braking to the dribbler.
		//
		void dribble_scram();

		//
		// Sets the power level of the dribbler. The parameter is a power level
		// between -1023 and 1023.
		//
		void dribble(int16_t power);

		//
		// Emitted whenever new statistics are available.
		//
		sigc::signal<void> &signal_updated() {
			return sig_updated;
		}

		//
		// Gets the average latency (in seconds) of an outbound packet to this robot.
		//
		double latency() const {
			return latency_;
		}

		//
		// Gets the fraction of the last 64 packets that were successfully delivered.
		// 
		double success_rate() const {
			return __builtin_popcount(tx_success_mask) / 64.0;
		}

		//
		// Checks whether a feedback packet has been received yet.
		//
		bool has_feedback() const {
			return !!(fb_packet.flags & xbeepacket::FEEDBACK_FLAG_RUNNING);
		}

		//
		// Gets the signal strength (in dBm) of the last outbound packet that
		// requested feedback.
		//
		int out_rssi() const {
			return -fb_packet.outbound_rssi;
		}

		//
		// Gets the signal strength (in dBm) of the last inbound packet.
		//
		int in_rssi() const {
			return -fb_packet.rxhdr.rssi;
		}

		//
		// Gets the speed of the dribbler.
		//
		uint16_t dribbler_speed() const {
			return fb_packet.dribbler_speed;
		}

		//
		// Gets the voltage (in volts) of the battery.
		//
		double battery_voltage() const {
			return fb_packet.battery_level / 1023.0 * 3.3 / 470.0 * (470.0 + 2200.0);
		}

		//
		// Gets the fault status of a drive motor.
		//
		bool drive_fault(unsigned int motor) const {
			assert(motor < 4);
			return !!(fb_packet.faults & (1 << motor));
		}

		//
		// Gets the fault status of the dribbler motor.
		//
		bool dribbler_fault() const {
			return !!(fb_packet.faults & (1 << 4));
		}

	private:
		xbee &modem;
		uint64_t address;
		bool started;
		sigc::signal<void> sig_updated;
		sigc::connection receive_connection;
		sigc::connection timeout_connection;
		xbeepacket::RUN_DATA out_packet;
		Glib::Timer out_packet_timer;
		double latency_;
		uint64_t tx_success_mask;
		xbeepacket::FEEDBACK_DATA fb_packet;
		unsigned int feedback_counter;

		void drive_impl(uint8_t flag, int16_t m1, int16_t m2, int16_t m3, int16_t m4);
		void on_receive(const void *, std::size_t);
		void on_timeout();
		void send_packet();
};

#endif

