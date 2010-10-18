#include "simulator/connection.h"
#include "simulator/sockproto/proto.h"
#include "util/exception.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

Simulator::Connection::Ptr Simulator::Connection::create(FileDescriptor::Ptr sock) {
	Ptr p(new Connection(sock));
	return p;
}

sigc::signal<void> &Simulator::Connection::signal_closed() const {
	return signal_closed_;
}

sigc::signal<void, const Simulator::Proto::A2SPacket &> &Simulator::Connection::signal_packet() const {
	return signal_packet_;
}

void Simulator::Connection::send(const Proto::S2APacket &packet) {
	if (::send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) < 0) {
		try {
			throw SystemError("send", errno);
		} catch (const SystemError &exp) {
			std::cout << exp.what() << '\n';
		}
		signal_closed().emit();
	}
}

Simulator::Connection::Connection(FileDescriptor::Ptr sock) : sock(sock) {
	Glib::signal_io().connect(sigc::mem_fun(this, &Connection::on_readable), sock->fd(), Glib::IO_IN);
}

Simulator::Connection::~Connection() {
}

bool Simulator::Connection::on_readable(Glib::IOCondition) {
	Proto::A2SPacket packet;
	iovec iov = { iov_base: &packet, iov_len: sizeof(packet), };
	msghdr mh = { msg_name: 0, msg_namelen: 0, msg_iov: &iov, msg_iovlen: 1, msg_control: 0, msg_controllen: 0, msg_flags: 0, };
	ssize_t rc = recvmsg(sock->fd(), &mh, MSG_DONTWAIT);
	if (rc < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		} else {
			try {
				throw SystemError("recvmsg", errno);
			} catch (const SystemError &exp) {
				std::cout << exp.what() << '\n';
			}
			signal_closed().emit();
			return false;
		}
	} else if (!rc) {
		std::cout << "Connection closed.\n";
		signal_closed().emit();
		return false;
	} else if (rc != sizeof(packet) || (mh.msg_flags & MSG_TRUNC)) {
		std::cout << "AI sent bad packet.\n";
		signal_closed().emit();
		return false;
	} else {
		// Might drop the last reference inside a signal_packet() handler; keep the object alive until we return.
		Ptr guard(this);
		signal_packet().emit(packet);
		return true;
	}
}

