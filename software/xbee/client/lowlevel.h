#ifndef XBEE_CLIENT_LOWLEVEL_H
#define XBEE_CLIENT_LOWLEVEL_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/shm.h"
#include "xbee/client/packet.h"
#include "xbee/shared/number_allocator.h"
#include "xbee/shared/packettypes.h"
#include <cstddef>
#include <stdint.h>
#include <glibmm.h>

//
// Allows access to the XBee radio modem, via the arbiter daemon.
//
class XBeeLowLevel : public NonCopyable {
	public:
		//
		// Connects to the arbiter daemon, launching it if necessary.
		//
		XBeeLowLevel();

		/**
		 * Claims the entire universe, if possible.
		 *
		 * \return \c true if the claim succeeded, or \c false if not.
		 */
		bool claim_universe();

		//
		// Sends a packet.
		//
		void send(XBeePacket::Ptr);

		//
		// Fired when a data packet is received with a 16-bit source address.
		//
		sigc::signal<void, uint16_t, uint8_t, const void *, std::size_t> signal_receive16;

		//
		// Fired when a meta packet is received.
		//
		sigc::signal<void, const void *, std::size_t> signal_meta;

	private:
		NumberAllocator<uint8_t> frame_allocator;
		const FileDescriptor::Ptr sock;
		XBeePacket::Ptr packets[256];

		bool on_readable(Glib::IOCondition);

	public:
		//
		// The shared memory block shared with the dæmon.
		//
		ShmBlock<XBeePacketTypes::SHM_BLOCK> shm;
};

#endif

