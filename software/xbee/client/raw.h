#ifndef XBEE_CLIENT_RAW_H
#define XBEE_CLIENT_RAW_H

#include "util/byref.h"
#include "xbee/client/packet.h"
#include <cstddef>
#include <glibmm.h>
#include <stdint.h>

class XBeeLowLevel;

//
// Allows access to a robot in raw mode. Raw mode is used to send and receive
// radio packets directly to and from the robot, for example when bootloading.
//
class XBeeRawBot : public ByRef {
	public:
		//
		// A pointer to an XBeeRawBot.
		//
		typedef RefPtr<XBeeRawBot> Ptr;

		//
		// The 64-bit address of this robot.
		//
		const uint64_t address;

		//
		// Fired when the arbiter has granted exclusive access to the robot.
		//
		sigc::signal<void> signal_alive;

		//
		// Fired if the claim request failed because another client already owns
		// the robot.
		//
		sigc::signal<void> signal_claim_failed;

		//
		// Fired when a data packet is received from this robot.
		//
		sigc::signal<void, uint8_t, const void *, std::size_t> signal_receive16;

		//
		// Creates a new XBeeRawBot and begins attempting to claim the bot.
		// You should connect to signal_alive to detect when you have been given
		// exclusive access to the robot. You should also connect to
		// signal_claim_failed to detect if your claim request is rejected. You
		// should gracefully handle the situation where neither signal has been
		// delivered yet, because it is possible for a nontrivial amount of time
		// to pass between the construction of the object and the delivery of
		// one of the signals if the arbiter is in the process of deassigning
		// allocated resources from the robot before granting it to you.
		//
		static Ptr create(uint64_t address, XBeeLowLevel &ll) {
			Ptr p(new XBeeRawBot(address, ll));
			return p;
		}

		//
		// Sends a XBeePacket to this robot. You are responsible for ensuring that
		// the robot's address is filled in properly and that you are not
		// accidentally sending data to the wrong robot!
		//
		void send(XBeePacket::Ptr p);

		/**
		 * \return The 16-bit address allocated to this robot
		 */
		uint16_t address16() const;

	private:
		XBeeLowLevel &ll;
		uint16_t address16_;

		XBeeRawBot(uint64_t address, XBeeLowLevel &ll);
		~XBeeRawBot();
		void on_receive16(uint16_t, uint8_t, const void *, std::size_t);
		void on_meta(const void *, std::size_t);
};

#endif

