#include "ai/backend/xbeed/refbox.h"
#include "util/dprint.h"
#include "util/sockaddrs.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace AI::BE::XBeeD;

namespace {
	FileDescriptor::Ptr create_socket() {
		const FileDescriptor::Ptr fd(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

		fd->set_blocking(false);

		const int one = 1;
		if (setsockopt(fd->fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
			throw std::runtime_error("Cannot set SO_REUSEADDR.");
		}

		SockAddrs sa;
		sa.in.sin_family = AF_INET;
		sa.in.sin_addr.s_addr = get_inaddr_any();
		sa.in.sin_port = htons(10001);
		std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
		if (bind(fd->fd(), &sa.sa, sizeof(sa.in)) < 0) {
			throw std::runtime_error("Cannot bind to port 10001 for refbox data.");
		}

		ip_mreqn mcreq;
		mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.1");
		mcreq.imr_address.s_addr = get_inaddr_any();
		mcreq.imr_ifindex = 0;
		if (setsockopt(fd->fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
			LOG_WARN("Cannot join multicast group 224.5.23.1 for refbox data.");
		}

		return fd;
	}
}

RefBox::RefBox() : command('H'), goals_blue(0), goals_yellow(0), fd(create_socket()) {
	Glib::signal_io().connect(sigc::mem_fun(this, &RefBox::on_readable), fd->fd(), Glib::IO_IN);
}

RefBox::~RefBox() {
}

bool RefBox::on_readable(Glib::IOCondition) {
	unsigned char packet[6];
	ssize_t len = recv(fd->fd(), &packet, sizeof(packet), 0);
	if (len < 0) {
		int err = errno;
		LOG_WARN(Glib::ustring::compose("Cannot receive from refbox socket: %1.", std::strerror(err)));
		return true;
	}
	if (len != static_cast<ssize_t>(sizeof(packet))) {
		LOG_WARN(Glib::ustring::compose("Refbox packet was %1 bytes, expected %2.", len, sizeof(packet)));
		return true;
	}

	command = packet[0];
	goals_blue = packet[1];
	goals_yellow = packet[2];

	return true;
}

