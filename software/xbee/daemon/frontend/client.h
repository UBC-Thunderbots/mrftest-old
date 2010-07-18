#ifndef XBEE_DAEMON_FRONTEND_CLIENT_H
#define XBEE_DAEMON_FRONTEND_CLIENT_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include "xbee/daemon/frontend/request.h"
#include "xbee/shared/packettypes.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <glibmm.h>
#include <stdint.h>

class XBeeDaemon;

//
// Manages all the state associated with a client's connection to this dæmon.
//
class XBeeClient : public NonCopyable, public sigc::trackable {
	public:
		//
		// Constructs a new XBeeClient.
		//
		static void create(FileDescriptor &sock, class XBeeDaemon &daemon);

		//
		// Sends a packet to all existent clients.
		//
		static void send_to_all(const void *data, std::size_t length);

		/**
		 * \return \c true if any clients are connected, or \c false if not.
		 */
		static bool any_connected();

	private:
		FileDescriptor sock;
		class XBeeDaemon &daemon;
		std::unordered_set<uint64_t> claimed;
		std::unordered_map<uint64_t, sigc::connection> pending_raw_claims;
		std::unordered_map<uint64_t, sigc::connection> alive_connections;
		std::unordered_map<uint64_t, sigc::connection> dead_connections;
		std::unordered_map<uint64_t, sigc::connection> feedback_connections;

		XBeeClient(FileDescriptor &, class XBeeDaemon &);
		~XBeeClient();
		void connect_frame_dealloc(XBeeRequest::ptr, uint8_t);
		void on_radio_packet(const std::vector<uint8_t> &);
		bool on_socket_ready(Glib::IOCondition);
		void on_packet(void *, std::size_t);
		void on_at_command(void *, std::size_t);
		void on_at_response(const void *, std::size_t, uint8_t);
		void on_remote_at_command(void *, std::size_t);
		void on_remote_at_response(const void *, std::size_t, uint8_t);
		void on_transmit(void *, std::size_t);
		void on_transmit_status(const void *, std::size_t, uint8_t);
		void on_meta(const void *, std::size_t);
		void on_meta_claim_universe();
		void on_meta_claim(const XBeePacketTypes::META_CLAIM &);
		void on_meta_release(const XBeePacketTypes::META_RELEASE &);
		void on_robot_alive(uint64_t);
		void on_robot_dead(uint64_t);
		void on_robot_feedback(uint64_t);
		void do_release(uint64_t);
};

#endif

