#define DEBUG 0
#include "util/dprint.h"
#include "xbee/daemon/frontend/backend.h"
#include "xbee/daemon/frontend/client.h"
#include "xbee/daemon/frontend/daemon.h"
#include "xbee/daemon/frontend/resource_allocation_failed.h"
#include "xbee/shared/packettypes.h"
#include <algorithm>
#include <functional>
#include <ext/functional>
#include <glibmm.h>
#include <cerrno>
#include <cstring>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace {
	std::tr1::unordered_set<client *> instances;
}

void client::create(file_descriptor &sock, class daemon &daemon) {
	new client(sock, daemon);
}

void client::send_to_all(const void *data, std::size_t length) {
	for (std::tr1::unordered_set<client *>::const_iterator i = instances.begin(), iend = instances.end(); i != iend; ++i) {
		send((*i)->sock, data, length, MSG_NOSIGNAL);
	}
}

client::client(file_descriptor &sck, class daemon &daemon) : sock(sck), daemon(daemon) {
	// Connect to signals.
	DPRINT("Accepted new client connection.");
	Glib::signal_io().connect(sigc::mem_fun(this, &client::on_socket_ready), sock, Glib::IO_IN | Glib::IO_HUP);
	daemon.backend.signal_received.connect(sigc::mem_fun(this, &client::on_radio_packet));

	// Record our own existence.
	instances.insert(this);

	// Send the welcome message.
	send(sock, "XBEE", 4, MSG_NOSIGNAL);

	// Send the second-level welcome with attached SHM file descriptor.
	iovec iov;
	iov.iov_base = const_cast<char *>("SHM");
	iov.iov_len = 3;
	char control_buffer[CMSG_SPACE(sizeof(int))];
	msghdr mh;
	mh.msg_name = 0;
	mh.msg_namelen = 0;
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;
	mh.msg_control = control_buffer;
	mh.msg_controllen = sizeof(control_buffer);
	mh.msg_flags = 0;
	cmsghdr *cmsg = CMSG_FIRSTHDR(&mh);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	reinterpret_cast<int *>(CMSG_DATA(cmsg))[0] = daemon.shm.fd();
	sendmsg(sock, &mh, MSG_NOSIGNAL);
}

client::~client() {
	DPRINT("Client connection closed.");
	std::for_each(claimed.begin(), claimed.end(), sigc::mem_fun(this, &client::do_release));
	instances.erase(this);
	if (instances.empty()) {
		daemon.last_client_disconnected();
	}
}

void client::connect_frame_dealloc(request::ptr req, uint8_t frame) {
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(daemon.frame_number_allocator, &number_allocator<uint8_t>::free), frame))));
}

void client::on_radio_packet(const std::vector<uint8_t> &data) {
	if (data[0] == xbeepacket::RECEIVE64_APIID || data[0] == xbeepacket::RECEIVE16_APIID) {
		if (send(sock, &data[0], data.size(), MSG_NOSIGNAL) != static_cast<ssize_t>(data.size())) {
			delete this;
		}
	}
}

bool client::on_socket_ready(Glib::IOCondition cond) {
	DPRINT("Socket ready.");
	if ((cond & Glib::IO_HUP) || !(cond & Glib::IO_IN)) {
		delete this;
		return false;
	}
	if (cond & Glib::IO_IN) {
		uint8_t buffer[65536];
		ssize_t ret = recv(sock, buffer, sizeof(buffer), MSG_DONTWAIT);
		DPRINT("Got packet.");
		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return true;
			}
			delete this;
			return false;
		}
		on_packet(buffer, static_cast<std::size_t>(ret));
	}
	return true;
}

void client::on_packet(void *buffer, std::size_t length) {
	// Check length.
	if (!length) {
		return;
	}

	// Dispatch by API ID.
	switch (static_cast<uint8_t *>(buffer)[0]) {
		case xbeepacket::AT_REQUEST_APIID:
		case xbeepacket::AT_REQUEST_QUEUE_APIID:
			DPRINT("Local AT command not implemented.");
			break;

		case xbeepacket::REMOTE_AT_REQUEST_APIID:
			on_remote_at_command(buffer, length);
			break;

		case xbeepacket::TRANSMIT64_APIID:
		case xbeepacket::TRANSMIT16_APIID:
			on_transmit(buffer, length);
			break;

		case xbeepacket::META_APIID:
			on_meta(buffer, length);
			break;

		default:
			DPRINT("Unknown packet type!");
			break;
	}
}

void client::on_remote_at_command(void *buffer, std::size_t length) {
	xbeepacket::REMOTE_AT_REQUEST<0> *packet = static_cast<xbeepacket::REMOTE_AT_REQUEST<0> *>(buffer);

	// Check packet size.
	if (length < sizeof(xbeepacket::REMOTE_AT_REQUEST<0>)) {
		DPRINT("Remote AT command with wrong packet length.");
		return;
	}

	// If a frame number was provided, reallocate it.
	uint8_t original_frame = packet->frame;
	if (original_frame) {
		packet->frame = daemon.frame_number_allocator.alloc();
	}

	// Create a request structure.
	request::ptr req(request::create(packet, length, original_frame != 0));

	// If a frame number was allocated, attach callbacks.
	if (original_frame) {
		// Attach a callback to deallocate the frame number.
		connect_frame_dealloc(req, packet->frame);
		// Attach a callback to notify this client.
		req->signal_complete().connect(sigc::bind(sigc::mem_fun(this, &client::on_remote_at_response), original_frame));
	}

	// Queue the request with the scheduler.
	daemon.scheduler.queue(req);
}

void client::on_remote_at_response(const void *buffer, std::size_t length, uint8_t original_frame) {
	const xbeepacket::REMOTE_AT_RESPONSE *packet = static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check API ID.
	if (packet->apiid != xbeepacket::REMOTE_AT_RESPONSE_APIID) {
		DPRINT("Remote AT response with wrong API ID.");
		return;
	}

	// Check packet size.
	if (length < sizeof(xbeepacket::REMOTE_AT_RESPONSE)) {
		DPRINT("Remote AT response with wrong packet length.");
		return;
	}

	// Allocate local copy of packet.
	uint8_t newpacket[length];
	std::memcpy(newpacket, packet, length);

	// Substitute in original frame number.
	reinterpret_cast<xbeepacket::REMOTE_AT_RESPONSE *>(newpacket)->frame = original_frame;

	// Send back-substituted packet to client.
	ssize_t ret = send(sock, newpacket, length, MSG_NOSIGNAL);
	if (ret != static_cast<ssize_t>(length)) {
		DPRINT("Cannot send data to client!");
		delete this;
		return;
	}
}

void client::on_transmit(void *buffer, std::size_t length) {
	xbeepacket::TRANSMIT64_HDR *packet = static_cast<xbeepacket::TRANSMIT64_HDR *>(buffer);

	// Check packet size.
	if (length < (packet->apiid == xbeepacket::TRANSMIT64_APIID ? 11 : 5)) {
		DPRINT("Transmit data command with wrong packet length.");
		return;
	}

	// If a frame number was provided, reallocate it.
	uint8_t original_frame = packet->frame;
	if (original_frame) {
		packet->frame = daemon.frame_number_allocator.alloc();
	}

	// Create a request structure.
	request::ptr req(request::create(packet, length, original_frame != 0));

	// If a frame number was allocated, attach callbacks.
	if (original_frame) {
		// Attach a callback to deallocate the frame number.
		connect_frame_dealloc(req, packet->frame);
		// Attach a callback to notify this client.
		req->signal_complete().connect(sigc::bind(sigc::mem_fun(this, &client::on_transmit_status), original_frame));
	}

	// Queue the request with the scheduler.
	daemon.scheduler.queue(req);
}

void client::on_transmit_status(const void *buffer, std::size_t length, uint8_t original_frame) {
	const xbeepacket::TRANSMIT_STATUS *packet = static_cast<const xbeepacket::TRANSMIT_STATUS *>(buffer);

	// Check API ID.
	if (packet->apiid != xbeepacket::TRANSMIT_STATUS_APIID) {
		DPRINT("Transmit status with wrong API ID.");
		return;
	}

	// Check packet size.
	if (length != sizeof(xbeepacket::TRANSMIT_STATUS)) {
		DPRINT("Transmit status with wrong packet length.");
		return;
	}

	// Allocate local copy of packet.
	uint8_t newpacket[length];
	std::memcpy(newpacket, packet, length);

	// Substitute in original frame number.
	reinterpret_cast<xbeepacket::TRANSMIT_STATUS *>(newpacket)->frame = original_frame;

	// Send back-substituted packet to client.
	if (send(sock, newpacket, length, MSG_NOSIGNAL) != static_cast<ssize_t>(length)) {
		DPRINT("Cannot send data to client!");
		delete this;
		return;
	}
}

void client::on_meta(const void *buffer, std::size_t length) {
	const xbeepacket::META_HDR *packet = static_cast<const xbeepacket::META_HDR *>(buffer);

	// Check length.
	if (length < sizeof(xbeepacket::META_HDR)) {
		DPRINT("Meta with wrong packet length.");
		return;
	}

	// Dispatch by meta type.
	switch (packet->metatype) {
		case xbeepacket::CLAIM_METATYPE:
			if (length == sizeof(xbeepacket::META_CLAIM)) {
				on_meta_claim(*static_cast<const xbeepacket::META_CLAIM *>(buffer));
			} else {
				DPRINT("Meta with wrong packet length.");
			}
			break;

		case xbeepacket::RELEASE_METATYPE:
			if (length == sizeof(xbeepacket::META_RELEASE)) {
				on_meta_release(*static_cast<const xbeepacket::META_RELEASE *>(buffer));
			} else {
				DPRINT("Meta with wrong packet length.");
			}
			break;

		default:
			DPRINT("Unknown meta type!");
			break;
	}
}

void client::on_meta_claim(const xbeepacket::META_CLAIM &req) {
	// Look up the state of the requested robot.
	robot_state::ptr state = daemon.robots[req.address];
	if (!state) {
		state = robot_state::create(req.address, daemon);
		daemon.robots[req.address] = state;
	}

	// If the robot is claimed, reject the request outright.
	if (state->claimed()) {
		xbeepacket::META_CLAIM_FAILED resp;
		resp.hdr.apiid = xbeepacket::META_APIID;
		resp.hdr.metatype = xbeepacket::CLAIM_FAILED_LOCKED_METATYPE;
		resp.address = req.address;
		if (send(sock, &resp, sizeof(resp), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(resp))) {
			DPRINT("Cannot send data to client!");
			delete this;
		}
		return;
	}

	// If we're asking for raw mode but the robot is still freeing resources
	// from its prior drive-mode claim, then we can't accept this request right
	// now; instead, we must wait until the resources have been deallocated. We
	// do this by attaching to a signal that fires when the resources are freed;
	// when that happens, we just rerun on_meta_claim() from the top with the
	// same packet (note that sigc::bind() takes a value-copy, not a reference).
	// Because we want to allow the client to cancel the pending claim request
	// by sending a META_RELEASE, we also keep a map (called pending_raw_claims)
	// from the robot address to the signal connection that will perform the
	// aforementioned reinvocation; a META_RELEASE will disconnect the signal
	// connection, thus preventing the robot from being claimed. If this client
	// has already requested this robot, just do nothing.
	if (state->freeing() && !req.drive_mode) {
		if (!pending_raw_claims.count(req.address)) {
			pending_raw_claims[req.address] = state->signal_resources_freed.connect(sigc::bind(sigc::mem_fun(this, &client::on_meta_claim), req));
		}
		return;
	}

	// Try to enter the requested mode.
	if (req.drive_mode) {
		try {
			state->enter_drive_mode(this);
			claimed.insert(req.address);
			std::tr1::unordered_map<uint64_t, sigc::connection>::iterator i;
			i = alive_connections.find(req.address);
			if (i != alive_connections.end()) {
				i->second.disconnect();
			}
			i = dead_connections.find(req.address);
			if (i != dead_connections.end()) {
				i->second.disconnect();
			}
			i = feedback_connections.find(req.address);
			if (i != feedback_connections.end()) {
				i->second.disconnect();
			}
			alive_connections[req.address] = state->signal_alive.connect(sigc::bind(sigc::mem_fun(this, &client::on_robot_alive), req.address));
			dead_connections[req.address] = state->signal_dead.connect(sigc::bind(sigc::mem_fun(this, &client::on_robot_dead), req.address));
			feedback_connections[req.address] = state->signal_feedback.connect(sigc::bind(sigc::mem_fun(this, &client::on_robot_feedback), req.address));
		} catch (const resource_allocation_failed &exp) {
			xbeepacket::META_CLAIM_FAILED resp;
			resp.hdr.apiid = xbeepacket::META_APIID;
			resp.hdr.metatype = xbeepacket::CLAIM_FAILED_RESOURCE_METATYPE;
			resp.address = req.address;
			if (send(sock, &resp, sizeof(resp), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(resp))) {
				DPRINT("Cannot send data to client!");
				delete this;
			}
		}
	} else {
		state->enter_raw_mode(this);
		claimed.insert(req.address);
		xbeepacket::META_ALIVE resp;
		resp.hdr.apiid = xbeepacket::META_APIID;
		resp.hdr.metatype = xbeepacket::ALIVE_METATYPE;
		resp.address = req.address;
		resp.shm_frame = 0xFF;
		if (send(sock, &resp, sizeof(resp), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(resp))) {
			DPRINT("Cannot send data to client!");
			delete this;
		}
	}
}

void client::on_meta_release(const xbeepacket::META_RELEASE &req) {
	if (claimed.count(req.address)) {
		do_release(req.address);
		claimed.erase(req.address);
	}
}

void client::on_robot_alive(uint64_t address) {
	assert(claimed.count(address));

	robot_state::ptr bot(daemon.robots[address]);

	xbeepacket::META_ALIVE packet;
	packet.hdr.apiid = xbeepacket::META_APIID;
	packet.hdr.metatype = xbeepacket::ALIVE_METATYPE;
	packet.address = address;
	packet.shm_frame = bot->run_data_index();
	if (send(sock, &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		DPRINT("Cannot send data to client!");
		delete this;
	}
}

void client::on_robot_dead(uint64_t address) {
	assert(claimed.count(address));

	xbeepacket::META_DEAD packet;
	packet.hdr.apiid = xbeepacket::META_APIID;
	packet.hdr.metatype = xbeepacket::DEAD_METATYPE;
	packet.address = address;
	if (send(sock, &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		DPRINT("Cannot send data to client!");
		delete this;
	}
}

void client::on_robot_feedback(uint64_t address) {
	assert(claimed.count(address));

	xbeepacket::META_FEEDBACK packet;
	packet.hdr.apiid = xbeepacket::META_APIID;
	packet.hdr.metatype = xbeepacket::FEEDBACK_METATYPE;
	packet.address = address;
	if (send(sock, &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		DPRINT("Cannot send data to client!");
		delete this;
	}
}

void client::do_release(uint64_t address) {
	daemon.robots[address]->release();
	std::tr1::unordered_map<uint64_t, sigc::connection> *maps[4] = {&pending_raw_claims, &alive_connections, &dead_connections, &feedback_connections};
	for (unsigned int i = 0; i < sizeof(maps) / sizeof(*maps); ++i) {
		std::tr1::unordered_map<uint64_t, sigc::connection>::iterator j = maps[i]->find(address);
		if (j != maps[i]->end()) {
			j->second.disconnect();
			maps[i]->erase(j);
		}
	}
}

