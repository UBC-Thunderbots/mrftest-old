

#include "ai/backend/vision/vision_socket.h"
#include "ai/common/time.h"
#include "util/dprint.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/exception.h"
#include "util/sockaddrs.h"
#include "ai/common/time.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include <cerrno>
#include <cstring>
#include <glibmm/main.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>



using AI::BE::Vision::VisionSocket;

VisionSocket::VisionSocket(int multicast_interface, const std::string &port) : sock(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {
	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	AddrInfoSet ai(nullptr, port.c_str(), &hints);



	//sock.set_blocking(false);


	const int one = 1;
	if (setsockopt(sock.fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		throw SystemError("setsockopt(SO_REUSEADDR)", errno);
	}


	if (bind(sock.fd(), ai.first()->ai_addr, ai.first()->ai_addrlen) < 0) {
		throw SystemError("bind(:10002)", errno);
	}


	ip_mreqn mcreq;
	mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.2");
	mcreq.imr_address.s_addr = get_inaddr_any();
	mcreq.imr_ifindex = multicast_interface;
	if (setsockopt(sock.fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
		LOG_INFO(u8"Cannot join multicast group 224.5.23.2 for vision data.");
	}
	//conn = Glib::signal_io().connect(sigc::mem_fun(this, &VisionSocket::receive_packet), sock.fd(), Glib::IO_IN);

	vision_thread = std::thread(&VisionSocket::vision_loop, this);
}


VisionSocket::~VisionSocket() {
	conn.disconnect();
}


bool VisionSocket::receive_packet(Glib::IOCondition) {
    // Receive a packet.
    uint8_t buffer[65536];
    ssize_t len = recv(sock.fd(), buffer, sizeof(buffer), 0);
    if (len < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    LOG_WARN(u8"Cannot receive packet from SSL-Vision.");
            }
            return true;
    }

    // Decode it.
    SSL_WrapperPacket packet;
    if (!packet.ParseFromArray(buffer, static_cast<int>(len))) {
	    LOG_WARN(u8"Received malformed SSL-Vision packet.");
	    return true;
    }

    // Pass it to any attached listeners.
    signal_vision_data.emit(packet);

    return true;

}



void VisionSocket::vision_loop(){

	sock.set_blocking(true);
	while(true){
		uint8_t buffer[65536];
		ssize_t len = recv(sock.fd(), buffer, sizeof(buffer), 0); //Blocks until packet received

		AI::Timestamp time_rec = std::chrono::steady_clock::now(); //Time packet was received at

		// Decode it.
		SSL_WrapperPacket packet;
		if (!packet.ParseFromArray(buffer, static_cast<int>(len))) {
			LOG_WARN(u8"Received malformed SSL-Vision packet.");
			continue;
		}

		packets_mutex.lock();
		vision_packets.push(std::make_pair(packet, time_rec));
		packets_mutex.unlock();
	}
}
