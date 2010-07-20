#ifndef XBEE_DAEMON_FRONTEND_DAEMON_H
#define XBEE_DAEMON_FRONTEND_DAEMON_H

#include "util/fd.h"
#include "util/shm.h"
#include "xbee/daemon/frontend/robot_state.h"
#include "xbee/daemon/frontend/scheduler.h"
#include "xbee/shared/number_allocator.h"
#include "xbee/shared/packettypes.h"
#include <unordered_map>
#include <vector>
#include <glibmm.h>

class BackEnd;

//
// A running XBee arbiter dæmon.
//
class XBeeDaemon : public sigc::trackable {
	public:
		//
		// Constructs a new XBeeDaemon.
		//
		XBeeDaemon(BackEnd &backend);

		//
		// Destroys a XBeeDaemon.
		//
		~XBeeDaemon();

		//
		// Allocates a new offset in the run data packet. Returns 0xFF on failure.
		//
		uint8_t alloc_rundata_index();

		//
		// Frees an offset in the run data packet.
		//
		void free_rundata_index(uint8_t);

		//
		// Indicates that the last client has disconnected.
		//
		void check_shutdown();

		//
		// The backend implementation.
		//
		BackEnd &backend;

		//
		// The shared memory block shared with clients.
		//
		ShmBlock<XBeePacketTypes::SHM_BLOCK> shm;

		//
		// An allocator that allocates frame numbers.
		//
		NumberAllocator<uint8_t> frame_number_allocator;

		//
		// An allocator that allocates 16-bit addresses.
		//
		NumberAllocator<uint16_t> id16_allocator;

		//
		// The XBeeScheduler that schedules delivery of packets.
		//
		XBeeScheduler scheduler;

		//
		// A signal fired when the last client disconnects from the XBeeDaemon. The
		// application may connect to this signal to terminate its process, or
		// may ignore it and keep running allowing further connections.
		//
		sigc::signal<void> signal_last_client_disconnected;

		//
		// All robots that are being tracked in some way by the arbiter.
		//
		std::unordered_map<uint64_t, RefPtr<XBeeRobot> > robots;

		//
		// The 64-bit address of the robot that has been assigned each run data
		// index (or zero of no robot is using the index).
		//
		uint64_t run_data_index_reverse[XBeePacketTypes::MAX_DRIVE_ROBOTS];

		/**
		 * Whether or not the XBeeDaemon has been exclusively claimed via a
		 * XBeePacketTypes::META_CLAIM_UNIVERSE request.
		 */
		bool universe_claimed;

	private:
		const FileDescriptor lock_file;
		const FileDescriptor listen_sock;
		std::vector<bool> allocated_rundata_indices;
		sigc::connection check_shutdown_firer;

		bool on_accept(Glib::IOCondition);
};

#endif

