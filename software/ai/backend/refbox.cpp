#include "ai/backend/refbox.h"
#include "util/codec.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/sockaddrs.h"
#include <cstring>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

using AI::BE::RefBox;

namespace {
	FileDescriptor create_socket(int multicast_interface) {
		addrinfo hints;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = 0;
		hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
		AddrInfoSet ai(0, "10003", &hints);

		FileDescriptor fd(FileDescriptor::create_socket(ai.first()->ai_family, ai.first()->ai_socktype, ai.first()->ai_protocol));

		fd.set_blocking(false);

		const int one = 1;
		if (setsockopt(fd.fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
			throw SystemError(u8"setsockopt(SO_REUSEADDR)", errno);
		}

		if (bind(fd.fd(), ai.first()->ai_addr, ai.first()->ai_addrlen) < 0) {
			throw SystemError(u8"bind(:10001)", errno);
		}

		ip_mreqn mcreq;
		mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.1");
		mcreq.imr_address.s_addr = get_inaddr_any();
		mcreq.imr_ifindex = multicast_interface;
		if (setsockopt(fd.fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
			LOG_WARN(u8"Cannot join multicast group 224.5.23.1 for refbox data.");
		}

		return fd;
	}
}

RefBox::RefBox(int multicast_interface) : fd(create_socket(multicast_interface)) {
	Glib::signal_io().connect(sigc::mem_fun(this, &RefBox::on_readable), fd.fd(), Glib::IO_IN);
}

bool RefBox::on_readable(Glib::IOCondition) {
	unsigned char buffer[65536];
	ssize_t len = recv(fd.fd(), &buffer, sizeof(buffer), 0);
	if (len < 0) {
		int err = errno;
		LOG_WARN(Glib::ustring::compose(u8"Cannot receive from refbox socket: %1.", std::strerror(err)));
		return true;
	}
	if (!packet.ParseFromArray(buffer, static_cast<int>(len))) {
		LOG_WARN(u8"Cannot parse refbox packet.");
		return true;
	}
	signal_packet.emit();

	return true;
}

