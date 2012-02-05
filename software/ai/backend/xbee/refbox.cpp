#include "ai/backend/xbee/refbox.h"
#include "ai/backend/xbee/refbox_packet.h"
#include "util/codec.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/sockaddrs.h"
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace AI::BE::XBee;

static_assert(RefboxPacket::BUFFER_SIZE == 6, "Bitcodec builds wrong refbox packet size.");

namespace {
	FileDescriptor create_socket(unsigned int multicast_interface) {
		FileDescriptor fd(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

		fd.set_blocking(false);

		const int one = 1;
		if (setsockopt(fd.fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
			throw SystemError("setsockopt(SO_REUSEADDR)", errno);
		}

		SockAddrs sa;
		sa.in.sin_family = AF_INET;
		sa.in.sin_addr.s_addr = get_inaddr_any();
		encode_u16(&sa.in.sin_port, 10001);
		std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
		if (bind(fd.fd(), &sa.sa, sizeof(sa.in)) < 0) {
			throw SystemError("bind(:10001)", errno);
		}

		ip_mreqn mcreq;
		mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.1");
		mcreq.imr_address.s_addr = get_inaddr_any();
		mcreq.imr_ifindex = multicast_interface;
		if (setsockopt(fd.fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
			LOG_WARN("Cannot join multicast group 224.5.23.1 for refbox data.");
		}

		return fd;
	}
}

RefBox::RefBox(unsigned int multicast_interface) : command('H'), goals_blue(0), goals_yellow(0), fd(create_socket(multicast_interface)) {
	Glib::signal_io().connect(sigc::mem_fun(this, &RefBox::on_readable), fd.fd(), Glib::IO_IN);
}

bool RefBox::on_readable(Glib::IOCondition) {
	unsigned char packet[65536];
	ssize_t len = recv(fd.fd(), &packet, sizeof(packet), 0);
	if (len < 0) {
		int err = errno;
		LOG_WARN(Glib::ustring::compose("Cannot receive from refbox socket: %1.", std::strerror(err)));
		return true;
	}
	signal_packet.emit(packet, len);
	if (static_cast<std::size_t>(len) != RefboxPacket::BUFFER_SIZE) {
		LOG_WARN(Glib::ustring::compose("Refbox packet was %1 bytes, expected %2.", len, RefboxPacket::BUFFER_SIZE));
		return true;
	}

	RefboxPacket decoded(packet);
	command = decoded.command;
	goals_blue = decoded.goals_blue;
	goals_yellow = decoded.goals_yellow;

	return true;
}

