#include "ai/world/refbox.h"
#include "util/dprint.h"
#include "util/sockaddrs.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace {
	FileDescriptor create_socket() {
		FileDescriptor fd(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		fd.set_blocking(false);

		const int one = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
			throw std::runtime_error("Cannot set SO_REUSEADDR.");
		}

		SockAddrs sa;
		sa.in.sin_family = AF_INET;
		sa.in.sin_addr.s_addr = get_inaddr_any();
		sa.in.sin_port = htons(10001);
		std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
		if (bind(fd, &sa.sa, sizeof(sa.in)) < 0) {
			throw std::runtime_error("Cannot bind to port 10001 for refbox data.");
		}

		ip_mreqn mcreq;
		mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.1");
		mcreq.imr_address.s_addr = get_inaddr_any();
		mcreq.imr_ifindex = 0;
		if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
			LOG_INFO("Cannot join multicast group 224.5.23.1 for refbox data.");
		}

		return fd;
	}
}

RefBox::RefBox() : fd(create_socket()), command_('H'), goals_blue_(0), goals_yellow_(0), time_remaining_(0) {
	Glib::signal_io().connect(sigc::mem_fun(this, &RefBox::on_readable), fd, Glib::IO_IN);
}

bool RefBox::on_readable(Glib::IOCondition) {
	struct __attribute__((packed)) {
		char cmd;
		unsigned char cmd_counter;
		unsigned char goals_blue;
		unsigned char goals_yellow;
		unsigned short time_remaining;
	} packet;
	ssize_t len = recv(fd, &packet, sizeof(packet), 0);
	if (len < 0) {
		int err = errno;
		LOG_WARN(Glib::ustring::compose("Cannot receive from refbox socket: %1.", std::strerror(err)));
		return true;
	}
	if (len != static_cast<ssize_t>(sizeof(packet))) {
		LOG_WARN(Glib::ustring::compose("Refbox packet was %1 bytes, expected %2.", len, sizeof(packet)));
		return true;
	}

	if (command_ != packet.cmd) {
		command_ = packet.cmd;
		signal_command_changed.emit();
	}

	goals_blue_ = packet.goals_blue;
	goals_yellow_ = packet.goals_yellow;
	time_remaining_ = ntohs(packet.time_remaining);

	return true;
}

