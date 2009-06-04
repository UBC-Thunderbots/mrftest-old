#include "datapool/RefBox.h"
#include "datapool/World.h"
#include "Log/Log.h"

#include <iostream>
#include <cerrno>
#include <cstring>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

namespace {
	int sock = -1;
	int lastCounter = -1;
	struct GameStatePacket {
		char cmd;                      // current referee command
		unsigned char cmd_counter;     // increments each time new command is set
		unsigned char goals_blue;      // current score for blue team
		unsigned char goals_yellow;    // current score for yellow team
		unsigned short time_remaining; // seconds remaining for current game stage (network byte order)
	} __attribute__((packed));
}

void RefBox::init() {
	sock = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		int err = errno;
		std::cerr << "Cannot create referee box socket: " << std::strerror(err) << '\n';
		return;
	}

	int ival = 1;
	if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ival, sizeof(ival)) < 0) {
		int err = errno;
		std::cerr << "Cannot set SO_REUSEADDR: " << std::strerror(err) << '\n';
		::close(sock);
		sock = -1;
		return;
	}

	union {
		sockaddr sa;
		sockaddr_in in;
	} sa;
	sa.in.sin_family = AF_INET;
	sa.in.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.in.sin_port = htons(10001);
	memset(&sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
	if (::bind(sock, &sa.sa, sizeof(sa.in)) < 0) {
		int err = errno;
		std::cerr << "Cannot bind referee box socket: " << std::strerror(err) << '\n';
		::close(sock);
		sock = -1;
		return;
	}

	ip_mreqn req;
	req.imr_multiaddr.s_addr = inet_addr("224.5.23.1");
	req.imr_address.s_addr = htonl(INADDR_ANY);
	req.imr_ifindex = 0;
	if (::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(req)) < 0) {
		int err = errno;
		std::cerr << "Cannot join multicast group: " << std::strerror(err) << '\n';
		::close(sock);
		sock = -1;
		return;
	}
}

void RefBox::update() {
	if (sock < 0)
		return;

	GameStatePacket pkt;
	ssize_t ret = ::recv(sock, &pkt, sizeof(pkt), MSG_DONTWAIT | MSG_TRUNC);
	if (ret == sizeof(pkt)) {
		if (pkt.cmd_counter != lastCounter) {
			lastCounter = pkt.cmd_counter;
			switch (pkt.cmd) {
				case 'H':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Halt\n";
					World::get().playType(PlayType::doNothing);
					break;
				case 'S':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Stop\n";
					World::get().playType(PlayType::stop);
					break;
				case ' ': // normal start
					Log::log(Log::LEVEL_INFO, "RefBox") << "Ready\n";
					if (World::get().playType() == PlayType::prepareKickoff)
						World::get().playType(PlayType::kickoff);
					else if (World::get().playType() == PlayType::preparePenaltyKick)
						World::get().playType(PlayType::penaltyKick);
					else World::get().playType(PlayType::play);
					break;
				case 's': // force start
					Log::log(Log::LEVEL_INFO, "RefBox") << "Start\n";
					World::get().playType(PlayType::play);
					break;
				case '1':
					Log::log(Log::LEVEL_INFO, "RefBox") << "First Half\n";
					break;
				case 'h':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Half Time\n";
					World::get().playType(PlayType::doNothing);
					break;
				case '2':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Second Half\n";
					break;
				case 'o':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Overtime 1\n";
					break;
				case 'O':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Overtime 2\n";
					break;
				case 'a':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Penalty Shootout\n";
					World::get().playType(PlayType::penaltyKickoff);
					break;
				case 'k':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Kickoff Yellow\n";
					World::get().playType(PlayType::prepareKickoff);
					World::get().team(0)->specialPossession(true);
					World::get().team(1)->specialPossession(false);
					break;
				case 'K':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Kickoff Blue\n";
					World::get().playType(PlayType::prepareKickoff);
					World::get().team(0)->specialPossession(false);
					World::get().team(1)->specialPossession(true);
					break;
				case 'p':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Penalty Yellow\n";
					World::get().playType(PlayType::preparePenaltyKick);
					World::get().team(0)->specialPossession(true);
					World::get().team(1)->specialPossession(false);
					break;
				case 'P':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Penalty Blue\n";
					World::get().playType(PlayType::preparePenaltyKick);
					World::get().team(0)->specialPossession(false);
					World::get().team(1)->specialPossession(true);
					break;
				case 'f':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Direct Free Kick Yellow\n";
					World::get().playType(PlayType::directFreeKick);
					World::get().team(0)->specialPossession(true);
					World::get().team(1)->specialPossession(false);
					break;
				case 'F':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Direct Free Kick Blue\n";
					World::get().playType(PlayType::directFreeKick);
					World::get().team(0)->specialPossession(false);
					World::get().team(1)->specialPossession(true);
					break;
				case 'i':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Indiriect Free Kick Yellow\n";
					World::get().team(0)->specialPossession(true);
					World::get().team(1)->specialPossession(false);
					World::get().playType(PlayType::indirectFreeKick);
					break;
				case 'I':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Indirect Free Kick Blue\n";
					World::get().team(0)->specialPossession(false);
					World::get().team(1)->specialPossession(true);
					World::get().playType(PlayType::indirectFreeKick);
					break;
				case 't':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Timeout Yellow\n";
					World::get().playType(PlayType::doNothing);
					World::get().team(0)->specialPossession(true);
					World::get().team(1)->specialPossession(false);
					break;
				case 'T':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Timeout Blue\n";
					World::get().playType(PlayType::doNothing);
					World::get().team(0)->specialPossession(false);
					World::get().team(1)->specialPossession(true);
					break;
				case 'z':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Timeout End\n";
					World::get().playType(PlayType::doNothing);
					break;
				case 'g':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Goal Yellow\n";
					break;
				case 'G':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Goal Blue\n";
					break;
				case 'd':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Ungoal Yellow\n";
					break;
				case 'D':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Ungoal Blue\n";
					break;
				case 'y':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Yellow Card Yellow\n";
					break;
				case 'Y':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Yellow Card Blue\n";
					break;
				case 'r':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Red Card Yellow\n";
					break;
				case 'R':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Red Card Blue\n";
					break;
			}
		}
	}
}

