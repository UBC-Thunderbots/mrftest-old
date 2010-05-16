#ifndef XBEE_DAEMON_FRONTEND_DAEMON_H
#define XBEE_DAEMON_FRONTEND_DAEMON_H

#include "util/fd.h"
#include "util/shm.h"
#include "xbee/daemon/frontend/robot_state.h"
#include "xbee/daemon/frontend/scheduler.h"
#include "xbee/shared/number_allocator.h"
#include "xbee/shared/packettypes.h"
#include <tr1/unordered_map>
#include <vector>
#include <glibmm.h>

class backend;

//
// A running XBee arbiter dæmon.
//
class daemon : public sigc::trackable {
	public:
		//
		// Constructs a new daemon.
		//
		daemon(class backend &backend);

		//
		// Destroys a daemon.
		//
		~daemon();

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
		void last_client_disconnected();

		//
		// The backend implementation.
		//
		class backend &backend;

		//
		// The shared memory block shared with clients.
		//
		shmblock<xbeepacket::SHM_BLOCK> shm;

		//
		// An allocator that allocates frame numbers.
		//
		number_allocator<uint8_t> frame_number_allocator;

		//
		// An allocator that allocates 16-bit addresses.
		//
		number_allocator<uint16_t> id16_allocator;

		//
		// The scheduler that schedules delivery of packets.
		//
		class scheduler scheduler;

		//
		// A signal fired when the last client disconnects from the daemon. The
		// application may connect to this signal to terminate its process, or
		// may ignore it and keep running allowing further connections.
		//
		sigc::signal<void> signal_last_client_disconnected;

		//
		// All robots that are being tracked in some way by the arbiter.
		//
		std::tr1::unordered_map<uint64_t, robot_state::ptr> robots;

		//
		// The 64-bit address of the robot that has been assigned each run data
		// index (or zero of no robot is using the index).
		//
		uint64_t run_data_index_reverse[xbeepacket::MAX_DRIVE_ROBOTS];

	private:
		const file_descriptor lock_file;
		const file_descriptor listen_sock;
		std::vector<bool> allocated_rundata_indices;
		sigc::connection last_client_disconnected_firer;

		bool on_accept(Glib::IOCondition);
};

#endif

