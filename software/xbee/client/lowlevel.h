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
class xbee_lowlevel : public noncopyable {
	public:
		//
		// Connects to the arbiter daemon, launching it if necessary.
		//
		xbee_lowlevel();

		//
		// Sends a packet.
		//
		void send(packet::ptr);

		//
		// Fired when a data packet is received with a 64-bit source address.
		//
		sigc::signal<void, uint64_t, uint8_t, const void *, std::size_t> signal_receive64;

		//
		// Fired when a meta packet is received.
		//
		sigc::signal<void, const void *, std::size_t> signal_meta;

	private:
		number_allocator<uint8_t> frame_allocator;
		const file_descriptor sock;
		packet::ptr packets[256];

		bool on_readable(Glib::IOCondition);

	public:
		//
		// The shared memory block shared with the dæmon.
		//
		shmblock<xbeepacket::SHM_BLOCK> shm;
};

#endif

