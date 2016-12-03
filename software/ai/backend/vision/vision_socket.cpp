

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
	std::cout << "In vision constr\n" << std::endl;
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


	std::cout << "Making Thread" << std::endl;
	vision_thread = std::thread(&VisionSocket::vision_loop, this);
	std::cout << "Made Thread" << std::endl;
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
	std::cout << "In thread\n" << std::endl;

	static constexpr std::size_t NUM_PATTERNS = 16; //TODO: Get this number better way


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

		/*
		const SSL_DetectionFrame &det(packet.detection());

		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> yellow_packet = det.robots_yellow();


		bool seen_this_frame[NUM_PATTERNS];
		std::fill_n(seen_this_frame, NUM_PATTERNS, false);
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(yellow_packet); // might need to be *packet instead
		for (std::size_t j = 0; j < static_cast<std::size_t>(rep.size()); ++j) {
			const SSL_DetectionRobot &detbot = rep.Get(static_cast<int>(j));
			if (detbot.has_robot_id()) {
				unsigned int pattern = detbot.robot_id();
				if (pattern < NUM_PATTERNS && !seen_this_frame[pattern]) {
					seen_this_frame[pattern] = true;
					if (detbot.has_orientation()) {
						bool neg = false;
						//TODO: Make friendly side negative
						Point pos((neg ? -detbot.x() : detbot.x()) / 1000.0, (neg ? -detbot.y() : detbot.y()) / 1000.0);
						Angle ori = (Angle::of_radians(detbot.orientation()) + (neg ? Angle::half() : Angle::zero())).angle_mod();
						if(counter > 15){
							counter = 0;
							std::cout << "ID = " << pattern << " x= " << pos.x << " y= " << pos.y << "\n" << std::endl;
						}
					} else {
						LOG_WARN(u8"Vision packet has robot with no orientation.");
					}


				}
			}
		}


		 */
		//TODO: call mrfdongle encode camera packet


		packets_mutex.lock();
		vision_packets.push(std::make_pair(packet, time_rec));
		packets_mutex.unlock();
	}
}
