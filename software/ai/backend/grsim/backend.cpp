#include "ai/backend/backend.h"
#include "ai/backend/grsim/player.h"
#include "ai/backend/ssl_vision/backend.h"
#include "ai/backend/ssl_vision/team.h"
#include "proto/grSim_Packet.pb.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/sockaddrs.h"
#include <functional>
#include <string>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace AI {
	namespace BE {
		namespace GRSim {
			class BackendFactory : public AI::BE::BackendFactory {
				public:
					BackendFactory();
					void create_backend(const std::vector<bool> &disable_cameras, const std::string &, int multicast_interface, std::function<void(Backend &)> cb) const;
			};

			extern BackendFactory grsim_backend_factory_instance;
		}
	}
}

namespace {
	class FriendlyTeam : public AI::BE::SSLVision::Team<AI::BE::GRSim::Player, AI::BE::Player> {
		public:
			FriendlyTeam(AI::BE::Backend &backend);

		protected:
			void create_member(unsigned int pattern);
	};

	class EnemyTeam : public AI::BE::SSLVision::Team<AI::BE::Robot, AI::BE::Robot> {
		public:
			EnemyTeam(AI::BE::Backend &backend);

		protected:
			void create_member(unsigned int pattern);
	};

	class Backend : public AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam> {
		public:
			Backend(const std::vector<bool> &disable_cameras, int multicast_interface);
			AI::BE::GRSim::BackendFactory &factory() const;
			FriendlyTeam &friendly_team();
			const FriendlyTeam &friendly_team() const;
			EnemyTeam &enemy_team();
			const EnemyTeam &enemy_team() const;

		private:
			FriendlyTeam friendly;
			EnemyTeam enemy;
			FileDescriptor grsim_socket;

			void send_packet(unsigned int);
	};
}

AI::BE::GRSim::BackendFactory::BackendFactory() : AI::BE::BackendFactory("grSim") {
}

void AI::BE::GRSim::BackendFactory::create_backend(const std::vector<bool> &disable_cameras, const std::string &, int multicast_interface, std::function<void(Backend &)> cb) const {
	::Backend backend(disable_cameras, multicast_interface);
	cb(backend);
}

AI::BE::GRSim::BackendFactory AI::BE::GRSim::grsim_backend_factory_instance;

FriendlyTeam::FriendlyTeam(AI::BE::Backend &backend) : AI::BE::SSLVision::Team<AI::BE::GRSim::Player, AI::BE::Player>(backend) {
}

void FriendlyTeam::create_member(unsigned int pattern) {
	members[pattern].create(pattern, std::ref(backend.ball()));
}

EnemyTeam::EnemyTeam(AI::BE::Backend &backend) : AI::BE::SSLVision::Team<AI::BE::Robot, AI::BE::Robot>(backend) {
}

void EnemyTeam::create_member(unsigned int pattern) {
	members[pattern].create(pattern);
}

Backend::Backend(const std::vector<bool> &disable_cameras, int multicast_interface) : AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>(disable_cameras, multicast_interface, "10020"), friendly(*this), enemy(*this), grsim_socket(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {
	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
	AddrInfoSet ai("127.0.0.1", "20011", &hints);

	grsim_socket.set_blocking(true);

	if (connect(grsim_socket.fd(), ai.first()->ai_addr, ai.first()->ai_addrlen) < 0) {
		throw SystemError("connect(:20011)", errno);
	}

	signal_post_tick().connect(sigc::mem_fun(this, &Backend::send_packet));

	// grSim doesn’t send field geometry—hardcode it.
	{
		static const double length = 6.05;
		static const double total_length = length + 2.0 * 0.25 + 2.0 * 0.425;
		static const double width = 4.05;
		static const double total_width = width + 2.0 * 0.25 + 2.0 * 0.425;
		static const double goal_width = 0.7;
		static const double centre_circle_radius = 0.5;
		static const double defense_area_radius = 0.8;
		static const double defense_area_stretch = 0.35;
		field_.update(length, total_length, width, total_width, goal_width, centre_circle_radius, defense_area_radius, defense_area_stretch);
	}
}

AI::BE::GRSim::BackendFactory &Backend::factory() const {
	return AI::BE::GRSim::grsim_backend_factory_instance;
}

FriendlyTeam &Backend::friendly_team() {
	return friendly;
}

const FriendlyTeam &Backend::friendly_team() const {
	return friendly;
}

EnemyTeam &Backend::enemy_team() {
	return enemy;
}

const EnemyTeam &Backend::enemy_team() const {
	return enemy;
}

void Backend::send_packet(unsigned int) {
	grSim_Packet packet;
	grSim_Commands *commands = packet.mutable_commands();
	commands->set_timestamp(0.0);
	commands->set_isteamyellow(friendly_colour() == AI::Common::Colour::YELLOW);
	for (std::size_t i = 0; i < friendly_team().size(); ++i) {
		AI::BE::GRSim::Player::Ptr player = friendly_team().get_backend_robot(i);
		player->encode_orders(*commands->add_robot_commands());
	}
	std::string buffer;
	packet.SerializeToString(&buffer);
	if (send(grsim_socket.fd(), buffer.data(), buffer.size(), MSG_NOSIGNAL) < 0) {
		throw SystemError("sendmsg", errno);
	}
}

