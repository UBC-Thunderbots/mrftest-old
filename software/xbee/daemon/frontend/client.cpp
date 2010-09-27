#include "xbee/daemon/frontend/client.h"
#include "util/misc.h"
#include "xbee/daemon/frontend/backend.h"
#include "xbee/daemon/frontend/daemon.h"
#include "xbee/daemon/frontend/resource_allocation_failed.h"
#include "xbee/shared/packettypes.h"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <functional>
#include <glibmm.h>
#include <stdint.h>
#include <ext/functional>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

namespace {
	std::unordered_set<XBeeClient *> instances;
}

void XBeeClient::create(FileDescriptor::Ptr sock, XBeeDaemon &daemon) {
	new XBeeClient(sock, daemon);
}

void XBeeClient::send_to_all(const void *data, std::size_t length) {
	for (std::unordered_set<XBeeClient *>::const_iterator i = instances.begin(), iend = instances.end(); i != iend; ++i) {
		send((*i)->sock->fd(), data, length, MSG_NOSIGNAL);
	}
}

bool XBeeClient::any_connected() {
	return !instances.empty();
}

XBeeClient::XBeeClient(FileDescriptor::Ptr sck, XBeeDaemon &daemon) : sock(sck), daemon(daemon) {
	// Connect to signals.
	Glib::signal_io().connect(sigc::mem_fun(this, &XBeeClient::on_socket_ready), sock->fd(), Glib::IO_IN | Glib::IO_HUP);
	daemon.backend.signal_received.connect(sigc::mem_fun(this, &XBeeClient::on_radio_packet));

	// Record our own existence.
	instances.insert(this);

	// Send the welcome message.
	send(sock->fd(), "XBEE", 4, MSG_NOSIGNAL);

	// Send the second-level welcome with attached SHM file descriptor.
	iovec iov;
	iov.iov_base = const_cast<char *>("SHM");
	iov.iov_len = 3;
	char control_buffer[cmsg_space(sizeof(int))];
	msghdr mh;
	mh.msg_name = 0;
	mh.msg_namelen = 0;
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;
	mh.msg_control = control_buffer;
	mh.msg_controllen = sizeof(control_buffer);
	mh.msg_flags = 0;
	cmsghdr *cmsg = cmsg_firsthdr(&mh);
	cmsg->cmsg_len = cmsg_len(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	static_cast<int *>(cmsg_data(cmsg))[0] = daemon.shm.fd();
	sendmsg(sock->fd(), &mh, MSG_NOSIGNAL);
}

XBeeClient::~XBeeClient() {
	std::for_each(claimed.begin(), claimed.end(), sigc::mem_fun(this, &XBeeClient::do_release));
	instances.erase(this);
	if (instances.empty()) {
		daemon.check_shutdown();
	}

	// This could only have been true if we were the one who claimed the universe and were the only client,
	// in which case we should unclaim the universe.
	daemon.universe_claimed = false;
}

void XBeeClient::connect_frame_dealloc(XBeeRequest::Ptr req, uint8_t frame) {
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(daemon.frame_number_allocator, &NumberAllocator<uint8_t>::free), frame))));
}

void XBeeClient::on_radio_packet(const std::vector<uint8_t> &data) {
	if (data[0] == XBeePacketTypes::RECEIVE64_APIID || data[0] == XBeePacketTypes::RECEIVE16_APIID) {
		if (send(sock->fd(), &data[0], data.size(), MSG_NOSIGNAL) != static_cast<ssize_t>(data.size())) {
			delete this;
		}
	}
}

bool XBeeClient::on_socket_ready(Glib::IOCondition cond) {
	if ((cond & Glib::IO_HUP) || !(cond & Glib::IO_IN)) {
		delete this;
		return false;
	}
	if (cond & Glib::IO_IN) {
		uint8_t buffer[65536];
		ssize_t ret = recv(sock->fd(), buffer, sizeof(buffer), MSG_DONTWAIT);
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

void XBeeClient::on_packet(void *buffer, std::size_t length) {
	// Check length.
	if (!length) {
		return;
	}

	// Dispatch by API ID.
	switch (static_cast<uint8_t *>(buffer)[0]) {
		case XBeePacketTypes::AT_REQUEST_APIID:
		case XBeePacketTypes::AT_REQUEST_QUEUE_APIID:
			on_at_command(buffer, length);
			break;

		case XBeePacketTypes::REMOTE_AT_REQUEST_APIID:
			on_remote_at_command(buffer, length);
			break;

		case XBeePacketTypes::TRANSMIT64_APIID:
		case XBeePacketTypes::TRANSMIT16_APIID:
			on_transmit(buffer, length);
			break;

		case XBeePacketTypes::META_APIID:
			on_meta(buffer, length);
			break;
	}
}

void XBeeClient::on_at_command(void *buffer, std::size_t length) {
	XBeePacketTypes::AT_REQUEST<0> *packet = static_cast<XBeePacketTypes::AT_REQUEST<0> *>(buffer);

	// Check packet size.
	if (length < sizeof(XBeePacketTypes::AT_REQUEST<0>)) {
		return;
	}

	// If a frame number was provided, reallocate it.
	uint8_t original_frame = packet->frame;
	if (original_frame) {
		packet->frame = daemon.frame_number_allocator.alloc();
	}

	// Create a XBeeRequest structure.
	XBeeRequest::Ptr req(XBeeRequest::create(packet, length, original_frame != 0));

	// If a frame number was allocated, attach callbacks.
	if (original_frame) {
		// Attach a callback to deallocate the frame number.
		connect_frame_dealloc(req, packet->frame);
		// Attach a callback to notify this client.
		req->signal_complete().connect(sigc::bind(sigc::mem_fun(this, &XBeeClient::on_at_response), original_frame));
	}

	// Queue the XBeeRequest with the XBeeScheduler.
	daemon.scheduler.queue(req);
}

void XBeeClient::on_at_response(const void *buffer, std::size_t length, uint8_t original_frame) {
	const XBeePacketTypes::AT_RESPONSE *packet = static_cast<const XBeePacketTypes::AT_RESPONSE *>(buffer);

	// Check API ID.
	if (packet->apiid != XBeePacketTypes::AT_RESPONSE_APIID) {
		return;
	}

	// Check packet size.
	if (length < sizeof(XBeePacketTypes::AT_RESPONSE)) {
		return;
	}

	// Allocate local copy of packet.
	uint8_t newpacket[length];
	std::memcpy(newpacket, packet, length);

	// Substitute in original frame number.
	newpacket[offsetof(XBeePacketTypes::AT_RESPONSE, frame)] = original_frame;

	// Send back-substituted packet to client.
	ssize_t ret = send(sock->fd(), newpacket, length, MSG_NOSIGNAL);
	if (ret != static_cast<ssize_t>(length)) {
		delete this;
		return;
	}
}

void XBeeClient::on_remote_at_command(void *buffer, std::size_t length) {
	XBeePacketTypes::REMOTE_AT_REQUEST<0> *packet = static_cast<XBeePacketTypes::REMOTE_AT_REQUEST<0> *>(buffer);

	// Check packet size.
	if (length < sizeof(XBeePacketTypes::REMOTE_AT_REQUEST<0>)) {
		return;
	}

	// If a frame number was provided, reallocate it.
	uint8_t original_frame = packet->frame;
	if (original_frame) {
		packet->frame = daemon.frame_number_allocator.alloc();
	}

	// Create a XBeeRequest structure.
	XBeeRequest::Ptr req(XBeeRequest::create(packet, length, original_frame != 0));

	// If a frame number was allocated, attach callbacks.
	if (original_frame) {
		// Attach a callback to deallocate the frame number.
		connect_frame_dealloc(req, packet->frame);
		// Attach a callback to notify this client.
		req->signal_complete().connect(sigc::bind(sigc::mem_fun(this, &XBeeClient::on_remote_at_response), original_frame));
	}

	// Queue the XBeeRequest with the XBeeScheduler.
	daemon.scheduler.queue(req);
}

void XBeeClient::on_remote_at_response(const void *buffer, std::size_t length, uint8_t original_frame) {
	const XBeePacketTypes::REMOTE_AT_RESPONSE *packet = static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(buffer);

	// Check API ID.
	if (packet->apiid != XBeePacketTypes::REMOTE_AT_RESPONSE_APIID) {
		return;
	}

	// Check packet size.
	if (length < sizeof(XBeePacketTypes::REMOTE_AT_RESPONSE)) {
		return;
	}

	// Allocate local copy of packet.
	uint8_t newpacket[length];
	std::memcpy(newpacket, packet, length);

	// Substitute in original frame number.
	newpacket[offsetof(XBeePacketTypes::REMOTE_AT_RESPONSE, frame)] = original_frame;

	// Send back-substituted packet to client.
	ssize_t ret = send(sock->fd(), newpacket, length, MSG_NOSIGNAL);
	if (ret != static_cast<ssize_t>(length)) {
		delete this;
		return;
	}
}

void XBeeClient::on_transmit(void *buffer, std::size_t length) {
	XBeePacketTypes::TRANSMIT64_HDR *packet = static_cast<XBeePacketTypes::TRANSMIT64_HDR *>(buffer);

	// Check packet size.
	if (length < (packet->apiid == XBeePacketTypes::TRANSMIT64_APIID ? 11 : 5)) {
		return;
	}

	// If a frame number was provided, reallocate it.
	uint8_t original_frame = packet->frame;
	if (original_frame) {
		packet->frame = daemon.frame_number_allocator.alloc();
	}

	// Create a XBeeRequest structure.
	XBeeRequest::Ptr req(XBeeRequest::create(packet, length, original_frame != 0));

	// If a frame number was allocated, attach callbacks.
	if (original_frame) {
		// Attach a callback to deallocate the frame number.
		connect_frame_dealloc(req, packet->frame);
		// Attach a callback to notify this client.
		req->signal_complete().connect(sigc::bind(sigc::mem_fun(this, &XBeeClient::on_transmit_status), original_frame));
	}

	// Queue the XBeeRequest with the XBeeScheduler.
	daemon.scheduler.queue(req);
}

void XBeeClient::on_transmit_status(const void *buffer, std::size_t length, uint8_t original_frame) {
	const XBeePacketTypes::TRANSMIT_STATUS *packet = static_cast<const XBeePacketTypes::TRANSMIT_STATUS *>(buffer);

	// Check API ID.
	if (packet->apiid != XBeePacketTypes::TRANSMIT_STATUS_APIID) {
		return;
	}

	// Check packet size.
	if (length != sizeof(XBeePacketTypes::TRANSMIT_STATUS)) {
		return;
	}

	// Allocate local copy of packet.
	uint8_t newpacket[length];
	std::memcpy(newpacket, packet, length);

	// Substitute in original frame number.
	newpacket[offsetof(XBeePacketTypes::TRANSMIT_STATUS, frame)] = original_frame;

	// Send back-substituted packet to client.
	if (send(sock->fd(), newpacket, length, MSG_NOSIGNAL) != static_cast<ssize_t>(length)) {
		delete this;
		return;
	}
}

void XBeeClient::on_meta(const void *buffer, std::size_t length) {
	const XBeePacketTypes::META_HDR *packet = static_cast<const XBeePacketTypes::META_HDR *>(buffer);

	// Check length.
	if (length < sizeof(XBeePacketTypes::META_HDR)) {
		return;
	}

	// Dispatch by meta type.
	switch (packet->metatype) {
		case XBeePacketTypes::CLAIM_UNIVERSE_METATYPE:
			if (length == sizeof(XBeePacketTypes::META_CLAIM_UNIVERSE)) {
				on_meta_claim_universe();
			}
			break;

		case XBeePacketTypes::CLAIM_METATYPE:
			if (length == sizeof(XBeePacketTypes::META_CLAIM)) {
				on_meta_claim(*static_cast<const XBeePacketTypes::META_CLAIM *>(buffer));
			}
			break;

		case XBeePacketTypes::RELEASE_METATYPE:
			if (length == sizeof(XBeePacketTypes::META_RELEASE)) {
				on_meta_release(*static_cast<const XBeePacketTypes::META_RELEASE *>(buffer));
			}
			break;
	}
}

void XBeeClient::on_meta_claim_universe() {
	// Only accept the claim request if nobody else is connected.
	if (instances.size() == 1) {
		daemon.universe_claimed = true;
		XBeePacketTypes::META_ALIVE pkt;
		pkt.hdr.apiid = XBeePacketTypes::META_APIID;
		pkt.hdr.metatype = XBeePacketTypes::ALIVE_METATYPE;
		pkt.address = 0;
		pkt.address16 = 0xFFFF;
		pkt.shm_frame = 0xFF;
		if (send(sock->fd(), &pkt, sizeof(pkt), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(pkt))) {
			delete this;
		}
	} else {
		XBeePacketTypes::META_CLAIM_FAILED pkt;
		pkt.hdr.apiid = XBeePacketTypes::META_APIID;
		pkt.hdr.metatype = XBeePacketTypes::CLAIM_FAILED_LOCKED_METATYPE;
		pkt.address = 0;
		if (send(sock->fd(), &pkt, sizeof(pkt), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(pkt))) {
			delete this;
		}
	}
}

void XBeeClient::on_meta_claim(const XBeePacketTypes::META_CLAIM &req) {
	// Look up the state of the requested robot.
	XBeeRobot::Ptr state = daemon.robots[req.address];
	if (!state.is()) {
		state = XBeeRobot::create(req.address, daemon);
		daemon.robots[req.address] = state;
	}

	// If the robot is claimed, reject the request outright.
	if (state->claimed()) {
		XBeePacketTypes::META_CLAIM_FAILED resp;
		resp.hdr.apiid = XBeePacketTypes::META_APIID;
		resp.hdr.metatype = XBeePacketTypes::CLAIM_FAILED_LOCKED_METATYPE;
		resp.address = req.address;
		if (send(sock->fd(), &resp, sizeof(resp), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(resp))) {
			delete this;
		}
		return;
	}

	// If we're asking for raw mode but the robot is still freeing resources from its prior drive-mode claim,
	// then we can't accept this request right now;
	// instead, we must wait until the resources have been deallocated.
	// We do this by attaching to a signal that fires when the resources are freed;
	// when that happens, we just rerun on_meta_claim() from the top with the same packet (note that sigc::bind() takes a value-copy, not a reference).
	// Because we want to allow the client to cancel the pending claim request by sending a META_RELEASE,
	// we also keep a map (called pending_raw_claims) from the robot address to the signal connection that will perform the aforementioned reinvocation;
	// a META_RELEASE will disconnect the signal connection, thus preventing the robot from being claimed.
	// If this client has already requested this robot, just do nothing.
	if (state->freeing() && !req.drive_mode) {
		if (!pending_raw_claims.count(req.address)) {
			pending_raw_claims[req.address] = state->signal_resources_freed.connect(sigc::bind(sigc::mem_fun(this, &XBeeClient::on_meta_claim), req));
		}
		return;
	}

	// Try to enter the requested mode.
	if (req.drive_mode) {
		try {
			state->enter_drive_mode(this);
			claimed.insert(req.address);
			std::unordered_map<uint64_t, sigc::connection>::iterator i;
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
			alive_connections[req.address] = state->signal_alive.connect(sigc::bind(sigc::mem_fun(this, &XBeeClient::on_robot_alive), req.address));
			dead_connections[req.address] = state->signal_dead.connect(sigc::bind(sigc::mem_fun(this, &XBeeClient::on_robot_dead), req.address));
			feedback_connections[req.address] = state->signal_feedback.connect(sigc::bind(sigc::mem_fun(this, &XBeeClient::on_robot_feedback), req.address));
		} catch (const ResourceAllocationFailed &exp) {
			XBeePacketTypes::META_CLAIM_FAILED resp;
			resp.hdr.apiid = XBeePacketTypes::META_APIID;
			resp.hdr.metatype = XBeePacketTypes::CLAIM_FAILED_RESOURCE_METATYPE;
			resp.address = req.address;
			if (send(sock->fd(), &resp, sizeof(resp), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(resp))) {
				delete this;
			}
		}
	} else {
		state->enter_raw_mode(this);
		claimed.insert(req.address);
		XBeePacketTypes::META_ALIVE resp;
		resp.hdr.apiid = XBeePacketTypes::META_APIID;
		resp.hdr.metatype = XBeePacketTypes::ALIVE_METATYPE;
		resp.address = req.address;
		resp.address16 = state->address16();
		resp.shm_frame = 0xFF;
		if (send(sock->fd(), &resp, sizeof(resp), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(resp))) {
			delete this;
		}
	}
}

void XBeeClient::on_meta_release(const XBeePacketTypes::META_RELEASE &req) {
	if (claimed.count(req.address)) {
		do_release(req.address);
		claimed.erase(req.address);
	}
}

void XBeeClient::on_robot_alive(uint64_t address) {
	assert(claimed.count(address));

	XBeeRobot::Ptr bot(daemon.robots[address]);

	XBeePacketTypes::META_ALIVE packet;
	packet.hdr.apiid = XBeePacketTypes::META_APIID;
	packet.hdr.metatype = XBeePacketTypes::ALIVE_METATYPE;
	packet.address = address;
	packet.address16 = 0;
	packet.shm_frame = bot->run_data_index();
	if (send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		delete this;
	}
}

void XBeeClient::on_robot_dead(uint64_t address) {
	assert(claimed.count(address));

	XBeePacketTypes::META_DEAD packet;
	packet.hdr.apiid = XBeePacketTypes::META_APIID;
	packet.hdr.metatype = XBeePacketTypes::DEAD_METATYPE;
	packet.address = address;
	if (send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		delete this;
	}
}

void XBeeClient::on_robot_feedback(uint64_t address) {
	assert(claimed.count(address));

	XBeePacketTypes::META_FEEDBACK packet;
	packet.hdr.apiid = XBeePacketTypes::META_APIID;
	packet.hdr.metatype = XBeePacketTypes::FEEDBACK_METATYPE;
	packet.address = address;
	if (send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		delete this;
	}
}

void XBeeClient::do_release(uint64_t address) {
	daemon.robots[address]->release();
	std::unordered_map<uint64_t, sigc::connection> *maps[4] = {&pending_raw_claims, &alive_connections, &dead_connections, &feedback_connections};
	for (unsigned int i = 0; i < sizeof(maps) / sizeof(*maps); ++i) {
		std::unordered_map<uint64_t, sigc::connection>::iterator j = maps[i]->find(address);
		if (j != maps[i]->end()) {
			j->second.disconnect();
			maps[i]->erase(j);
		}
	}
}

