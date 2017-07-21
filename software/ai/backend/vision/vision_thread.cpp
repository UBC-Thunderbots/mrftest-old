#include "ai/backend/vision/vision_thread.h"
#include "mrf/dongle.h"
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
#include <fstream>


using AI::BE::Vision::VisionThread;
using AI::BE::Vision::VisionInfo;

bool VisionInfo::dataValid(){
	return data_valid;
}

bool VisionInfo::defendingEast(){
	return defending_east;
}
bool VisionInfo::friendlyIsYellow(){
	return friendly_yellow;
}
Point VisionInfo::ballPos(){
	std::lock_guard<std::mutex> lock(ball_mtx);
	return ball_pos;
}

void VisionInfo::setDataValid(bool new_val){
	data_valid = new_val;
}						
void VisionInfo::setDefendingEast(bool new_val){
	defending_east = new_val;
}
void VisionInfo::setFriendlyIsYellow(bool new_val){
	friendly_yellow = new_val;
}
void VisionInfo::setBallPos(Point new_val){
	std::lock_guard<std::mutex> lock(ball_mtx);
	ball_pos = new_val;
}





VisionThread::VisionThread(MRFDongle &dongle_,int multicast_interface, const std::string &port, const std::vector<bool> &disable_cameras_): dongle(dongle_), disable_cameras(disable_cameras_){
	vision_thread = std::thread(&VisionThread::vision_loop, this, multicast_interface, port);
}

VisionThread::~VisionThread() {
	//might need to do something here to make sure the thread dies before the dongle does
}


void VisionThread::vision_loop(int multicast_interface, std::string port){
	FileDescriptor sock(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)); 
	sock_init(sock, multicast_interface, port);
	
	while(stop_thread == false){
		uint8_t buffer[65536];
		ssize_t len = recv(sock.fd(), buffer, sizeof(buffer), 0); //Blocks until packet received

		// Decode it.
		SSL_WrapperPacket packet;
		if (!packet.ParseFromArray(buffer, static_cast<int>(len))) {
			// TODO: check if LOG_WARN is thread safe
			LOG_WARN(u8"Received malformed SSL-Vision packet.");
			continue;
		}
		if(vis_inf.dataValid()){
			transmit_bots(packet);
		}	
 		
		packets_mutex.lock();
		if(vision_packets.size() > 16){
			std::cout << "warning- throwing out vision packet- queue has 16 existing packets" << std::endl;
			vision_packets.pop(); // Too many packets, get rid of the oldest
		}
		vision_packets.push(packet);
		packets_mutex.unlock();
	}
}


void VisionThread::sock_init(FileDescriptor &sock, int multicast_interface, std::string port){
	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	AddrInfoSet ai(nullptr, port.c_str(), &hints);

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
		// TODO: check if LOG_INFO is thread safe
		LOG_INFO(u8"Cannot join multicast group 224.5.23.2 for vision data.");
	}
	sock.set_blocking(true);
}
void VisionThread::transmit_bots(SSL_WrapperPacket packet){
	if(!vis_inf.dataValid()) return;
	if(!packet.has_detection()) return;

	SSL_DetectionFrame det = packet.detection();
	std::vector<std::tuple<uint8_t,Point, Angle>> detbots;

	// if friendly team is yellow grab yellow robots, otherwise grab blue
	const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep = vis_inf.friendlyIsYellow() ? det.robots_yellow() : det.robots_blue(); 

	bool seen_this_frame[MAX_PATTERN] = {false};
	for (std::size_t j = 0; j < static_cast<std::size_t>(rep.size()); ++j) {
		const SSL_DetectionRobot &detbot = rep.Get(static_cast<int>(j));
		if (!detbot.has_robot_id()) continue;
		if (!detbot.has_orientation()){
			LOG_WARN(u8"Vision packet has robot with no orientation.");
			continue;
		}
		unsigned int pattern = detbot.robot_id();
		if((pattern >= MAX_PATTERN) || seen_this_frame[pattern]){
			 continue;
		}else{
			 seen_this_frame[pattern] = true;
		}

		// if defending east side, flip the sign of all points (defending end is always negative)
		bool neg = vis_inf.defendingEast();	
		Point pos((neg ? -detbot.x() : detbot.x()) / 1000.0, (neg ? -detbot.y() : detbot.y()) / 1000.0);
		Angle ori = (Angle::of_radians(detbot.orientation()) + (neg ? Angle::half() : Angle::zero())).angle_mod();
		detbots.push_back(std::make_tuple(pattern, pos, ori));
	}

	if(detbots.empty()) return;

	int64_t t_capture = 0;
	if(det.has_t_capture()){
		t_capture = (int64_t)det.t_capture();
	}
/*
	std::cout << "Calling dongle.send_camera_packet with: ";
	for (std::size_t i = 0; i < detbots.size(); ++i) {
		std::cout << "bot number = " << unsigned(std::get<0>(detbots[i])) << ", ";
		std::cout << "x = " << (std::get<1>(detbots[i])).x << ", ";
		std::cout << "y = " << (std::get<1>(detbots[i])).y << ", ";
		std::cout << "time capture = " << t_capture << ", ";
		std::cout << "theta = " << (std::get<2>(detbots[i])).to_degrees() << std::endl;
	}
*/
	Point ball_pos = vis_inf.ballPos();
 	
	//only send every fourth packet
	//TODO: make this for each camera
	if(cam_count == 0){
		dongle.send_camera_packet(detbots, ball_pos, t_capture);
		cam_count = 2;
	}else{
		cam_count --;
	}
}



