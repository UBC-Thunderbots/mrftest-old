#include "util/misc.h"
#include "util/sockaddrs.h"
#include "util/xbee.h"
#include "xbee/client/lowlevel.h"
#include "xbee/shared/packettypes.h"
#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

namespace {
	bool connect_to_existing_daemon(file_descriptor &sock) {
		// Calculate path.
		const Glib::ustring &cache_dir = Glib::get_user_cache_dir();
		const std::string &socket_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-sock");

		// Create a socket.
		file_descriptor tmp(PF_UNIX, SOCK_SEQPACKET, 0);
		sockaddrs sa;
		sa.un.sun_family = AF_UNIX;
		if (socket_path.size() > sizeof(sa.un.sun_path)) throw std::runtime_error("Path too long!");
		std::copy(socket_path.begin(), socket_path.end(), &sa.un.sun_path[0]);
		std::fill(&sa.un.sun_path[socket_path.size()], &sa.un.sun_path[sizeof(sa.un.sun_path) / sizeof(*sa.un.sun_path)], '\0');
		if (connect(tmp, &sa.sa, sizeof(sa.un)) < 0) {
			if (errno == ECONNREFUSED) return false;
			throw std::runtime_error("Cannot connect to arbiter daemon!");
		}

		// Read the signature string from the daemon. If this fails, we may have
		// hit the race condition when the daemon is dying.
		char buffer[4];
		if (recv(tmp, buffer, sizeof(buffer), 0) != sizeof(buffer))
			return false;
		if (buffer[0] != 'X' || buffer[1] != 'B' || buffer[2] != 'E' || buffer[3] != 'E')
			return false;

		// Return the socket.
		sock = tmp;
		return true;
	}

	void launch_daemon() {
		// Find out the path name to myself.
		std::vector<char> buffer(8);
		ssize_t ssz;
		while ((ssz = readlink("/proc/self/exe", &buffer[0], buffer.size())) == static_cast<ssize_t>(buffer.size())) {
			buffer.resize(buffer.size() * 2);
		}
		if (ssz < 0) {
			throw std::runtime_error("Cannot read executable path!");
		}
		std::string my_path(&buffer[0], &buffer[ssz]);

		// Compute the path name to the arbiter daemon.
		std::string xbeed_path = Glib::path_get_dirname(my_path);
		xbeed_path.append("/xbeed");

		// Launch the arbiter.
		std::vector<std::string> args;
		args.push_back(xbeed_path);
		args.push_back("--daemon");
		std::string output, error;
		int status;
		Glib::spawn_sync("", args, Glib::SpawnFlags(0), sigc::slot<void>(), &output, &error, &status);
		std::cout << output;
		std::cerr << error;
		if (!wifexited(status) || wexitstatus(status) != 0) {
			throw std::runtime_error("Cannot launch arbiter daemon!");
		}
	}

	file_descriptor connect_to_daemon() {
		file_descriptor sock;

		// Loop forever, until something works.
		for (;;) {
			// Try connecting to an already-running daemon.
			if (connect_to_existing_daemon(sock))
				return sock;

			// Try launching a new daemon.
			launch_daemon();
		}
	}

	file_descriptor receive_shm_fd(const file_descriptor &sock) {
		char databuf[3];
		iovec iov;
		iov.iov_base = databuf;
		iov.iov_len = 3;
		char controlbuf[cmsg_space(sizeof(int))];
		msghdr mh;
		mh.msg_name = 0;
		mh.msg_namelen = 0;
		mh.msg_iov = &iov;
		mh.msg_iovlen = 1;
		mh.msg_control = controlbuf;
		mh.msg_controllen = sizeof(controlbuf);
		mh.msg_flags = 0;
		if (recvmsg(sock, &mh, 0) != 3) {
			throw std::runtime_error("Cannot receive from socket!");
		}
		if (databuf[0] != 'S' || databuf[1] != 'H' || databuf[2] != 'M') {
			throw std::runtime_error("Received wrong message on socket!");
		}
		cmsghdr *cmsg = cmsg_firsthdr(&mh);
		if (cmsg->cmsg_len != cmsg_len(sizeof(int)) || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
			throw std::runtime_error("Received wrong ancillary data on socket!");
		}
		return file_descriptor(static_cast<const int *>(cmsg_data(cmsg))[0]);
	}
}

xbee_lowlevel::xbee_lowlevel() : frame_allocator(1, 255), sock(connect_to_daemon()), shm(receive_shm_fd(sock)) {
	Glib::signal_io().connect(sigc::mem_fun(this, &xbee_lowlevel::on_readable), sock, Glib::IO_IN | Glib::IO_HUP);
}

bool xbee_lowlevel::claim_universe() {
	xbeepacket::META_CLAIM_UNIVERSE req;
	req.hdr.apiid = xbeepacket::META_APIID;
	req.hdr.metatype = xbeepacket::CLAIM_UNIVERSE_METATYPE;
	if (::send(sock, &req, sizeof(req), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(req))) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}

	for (;;) {
		unsigned char buffer[65536];
		ssize_t ret = recv(sock, buffer, sizeof(buffer), 0);
		if (ret < 0) {
			throw std::runtime_error("Cannot talk to XBee arbiter!");
		}
		if (!ret) {
			throw std::runtime_error("XBee arbiter died!");
		}
		if (buffer[0] == xbeepacket::META_APIID) {
			if (buffer[1] == xbeepacket::ALIVE_METATYPE) {
				return true;
			} else if (buffer[1] == xbeepacket::CLAIM_FAILED_LOCKED_METATYPE) {
				return false;
			}
		}
	}
}

void xbee_lowlevel::send(packet::ptr pkt) {
	if (pkt->has_response) {
		uint8_t frame = frame_allocator.alloc();
		packets[frame] = pkt;
		pkt->transmit(sock, frame);
	} else {
		pkt->transmit(sock, 0);
	}
}

bool xbee_lowlevel::on_readable(Glib::IOCondition cond) {
	if (cond & Glib::IO_HUP) {
		throw std::runtime_error("XBee arbiter died!");
	}
	if (cond & (Glib::IO_ERR | Glib::IO_NVAL)) {
		throw std::runtime_error("File descriptor error!");
	}
	unsigned char buffer[65536];
	ssize_t ret = recv(sock, buffer, sizeof(buffer), 0);
	if (ret < 0) {
		throw std::runtime_error("Cannot talk to XBee arbiter!");
	}
	if (!ret) {
		throw std::runtime_error("XBee arbiter died!");
	}

	if (buffer[0] == xbeepacket::RECEIVE16_APIID) {
		const xbeepacket::RECEIVE16_HDR *hdr = reinterpret_cast<const xbeepacket::RECEIVE16_HDR *>(buffer);
		uint16_t address = (hdr->address[0] << 8) | hdr->address[1];
		signal_receive16.emit(address, hdr->rssi, &buffer[sizeof(*hdr)], ret - sizeof(*hdr));
	} else if (buffer[0] == xbeepacket::TRANSMIT_STATUS_APIID) {
		const xbeepacket::TRANSMIT_STATUS *pkt = reinterpret_cast<const xbeepacket::TRANSMIT_STATUS *>(buffer);
		if (packets[pkt->frame]) {
			packets[pkt->frame]->signal_complete().emit(pkt, ret);
			packets[pkt->frame].reset();
			frame_allocator.free(pkt->frame);
		}
	} else if (buffer[0] == xbeepacket::REMOTE_AT_RESPONSE_APIID) {
		const xbeepacket::REMOTE_AT_RESPONSE *pkt = reinterpret_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);
		if (packets[pkt->frame]) {
			packets[pkt->frame]->signal_complete().emit(pkt, ret);
			packets[pkt->frame].reset();
			frame_allocator.free(pkt->frame);
		}
	} else if (buffer[0] == xbeepacket::AT_RESPONSE_APIID) {
		const xbeepacket::AT_RESPONSE *pkt = reinterpret_cast<const xbeepacket::AT_RESPONSE *>(buffer);
		if (packets[pkt->frame]) {
			packets[pkt->frame]->signal_complete().emit(pkt, ret);
			packets[pkt->frame].reset();
			frame_allocator.free(pkt->frame);
		}
	} else if (buffer[0] == xbeepacket::META_APIID) {
		signal_meta.emit(buffer, ret);
	}

	return true;
}

