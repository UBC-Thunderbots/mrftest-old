#include "datapool/Config.h"
#include "datapool/RefBox.h"
#include "datapool/World.h"
#include "Log/Log.h"

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace {
	struct GameStatePacket {
		char cmd;                      // current referee command
		unsigned char cmd_counter;     // increments each time new command is set
		unsigned char goals_blue;      // current score for blue team
		unsigned char goals_yellow;    // current score for yellow team
		unsigned short time_remaining; // seconds remaining for current game stage (network byte order)
	} __attribute__((packed));
}

RefBox::RefBox() : fd(-1), last_count(-1) {
	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "RefBox") << "Cannot create socket: " << std::strerror(err) << '\n';
		std::exit(1);
	}

	int ival = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ival, sizeof(ival)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "RefBox") << "Cannot set SO_REUSEADDR: " << std::strerror(err) << '\n';
		close(fd);
		std::exit(1);
	}

	union {
		sockaddr sa;
		sockaddr_in in;
	} sa;
	sa.in.sin_family = AF_INET;
	sa.in.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.in.sin_port = htons(10001);
	memset(&sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
	if (bind(fd, &sa.sa, sizeof(sa.in)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "RefBox") << "Cannot bind socket: " << std::strerror(err) << '\n';
		close(fd);
		std::exit(1);
	}

	ip_mreqn req;
	req.imr_multiaddr.s_addr = inet_addr("224.5.23.1");
	req.imr_address.s_addr = htonl(INADDR_ANY);
	req.imr_ifindex = 0;
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(req)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "RefBox") << "Cannot join multicast group: " << std::strerror(err) << '\n';
		close(fd);
		std::exit(1);
	}

	Glib::signal_io().connect(sigc::mem_fun(*this, &RefBox::onIO), fd, Glib::IO_IN);
}

RefBox::~RefBox() {
	close(fd);
}

bool RefBox::onIO(Glib::IOCondition cond) {
	//World::get().playType(PlayType::play);
	//return false;

	if (cond & (Glib::IO_ERR | Glib::IO_NVAL | Glib::IO_HUP)) {
		Log::log(Log::LEVEL_ERROR, "RefBox") << "Error detected on socket\n";
		return false;
	}

	GameStatePacket pkt;
	ssize_t ret = recv(fd, &pkt, sizeof(pkt), MSG_DONTWAIT | MSG_TRUNC);
	if (ret == sizeof(pkt)) {
		const std::string &friendlyColour = Config::instance().getString("Game", "FriendlyColour");
		if (friendlyColour == "Yellow") {
			World::get().friendlyTeam().score(pkt.goals_yellow);
			World::get().enemyTeam().score(pkt.goals_blue);
		} else if (friendlyColour == "Blue") {
			World::get().friendlyTeam().score(pkt.goals_blue);
			World::get().enemyTeam().score(pkt.goals_yellow);
		} else {
			Log::log(Log::LEVEL_ERROR, "RefBox") << "Illegal config directive [Game]/FriendlyColour, should be Blue or Yellow.\n";
		}

		if (pkt.cmd_counter != last_count) {
			last_count = pkt.cmd_counter;
			switch (pkt.cmd) {
				case 'H':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Halt\n";
					World::get().playType(PlayType::doNothing);
					break;
				case 'S':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Stop\n";
					World::get().playType(PlayType::stop);
					World::get().team(0).specialPossession(false);
					World::get().team(1).specialPossession(false);
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
					break;
				case 'k':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Kickoff Yellow\n";
					World::get().playType(PlayType::prepareKickoff);
					World::get().team(0).specialPossession(true);
					World::get().team(1).specialPossession(false);
					break;
				case 'K':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Kickoff Blue\n";
					World::get().playType(PlayType::prepareKickoff);
					World::get().team(0).specialPossession(false);
					World::get().team(1).specialPossession(true);
					break;
				case 'p':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Penalty Yellow\n";
					World::get().playType(PlayType::preparePenaltyKick);
					World::get().team(0).specialPossession(true);
					World::get().team(1).specialPossession(false);
					break;
				case 'P':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Penalty Blue\n";
					World::get().playType(PlayType::preparePenaltyKick);
					World::get().team(0).specialPossession(false);
					World::get().team(1).specialPossession(true);
					break;
				case 'f':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Direct Free Kick Yellow\n";
					World::get().playType(PlayType::directFreeKick);
					World::get().team(0).specialPossession(true);
					World::get().team(1).specialPossession(false);
					break;
				case 'F':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Direct Free Kick Blue\n";
					World::get().playType(PlayType::directFreeKick);
					World::get().team(0).specialPossession(false);
					World::get().team(1).specialPossession(true);
					break;
				case 'i':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Indiriect Free Kick Yellow\n";
					World::get().team(0).specialPossession(true);
					World::get().team(1).specialPossession(false);
					World::get().playType(PlayType::indirectFreeKick);
					break;
				case 'I':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Indirect Free Kick Blue\n";
					World::get().team(0).specialPossession(false);
					World::get().team(1).specialPossession(true);
					World::get().playType(PlayType::indirectFreeKick);
					break;
				case 't':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Timeout Yellow\n";
					World::get().playType(PlayType::doNothing);
					World::get().team(0).specialPossession(true);
					World::get().team(1).specialPossession(false);
					break;
				case 'T':
					Log::log(Log::LEVEL_INFO, "RefBox") << "Timeout Blue\n";
					World::get().playType(PlayType::doNothing);
					World::get().team(0).specialPossession(false);
					World::get().team(1).specialPossession(true);
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

	return true;
}

