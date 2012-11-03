#include "ai/backend/physical/player.h"
#include "ai/backend/ssl_vision/backend.h"
#include "ai/backend/ssl_vision/team.h"
#include "xbee/dongle.h"
#include "xbee/robot.h"

using namespace AI::BE;

namespace {
	/**
	 * \brief The friendly team.
	 */
	class FriendlyTeam : public AI::BE::SSLVision::Team<AI::BE::Physical::Player, AI::BE::Player> {
		public:
			explicit FriendlyTeam(Backend &backend, XBeeDongle &dongle);

		protected:
			void create_member(unsigned int pattern);

		private:
			XBeeDongle &dongle;
	};

	/**
	 * \brief The enemy team.
	 */
	class EnemyTeam : public AI::BE::SSLVision::Team<AI::BE::Robot, AI::BE::Robot> {
		public:
			explicit EnemyTeam(Backend &backend);

		protected:
			void create_member(unsigned int pattern);
	};

	/**
	 * \brief The backend.
	 */
	class XBeeBackend : public AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam> {
		public:
			explicit XBeeBackend(XBeeDongle &dongle, unsigned int camera_mask, int multicast_interface);
			BackendFactory &factory() const;
			FriendlyTeam &friendly_team();
			const FriendlyTeam &friendly_team() const;
			EnemyTeam &enemy_team();
			const EnemyTeam &enemy_team() const;

		private:
			FriendlyTeam friendly;
			EnemyTeam enemy;
	};

	class XBeeBackendFactory : public BackendFactory {
		public:
			explicit XBeeBackendFactory();
			void create_backend(const std::string &, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const;
	};
}

XBeeBackendFactory xbee_backend_factory_instance;

FriendlyTeam::FriendlyTeam(Backend &backend, XBeeDongle &dongle) : AI::BE::SSLVision::Team<AI::BE::Physical::Player, AI::BE::Player>(backend), dongle(dongle) {
}

void FriendlyTeam::create_member(unsigned int pattern) {
	members.create(pattern, pattern, std::ref(dongle.robot(pattern)));
}

EnemyTeam::EnemyTeam(Backend &backend) : AI::BE::SSLVision::Team<AI::BE::Robot, AI::BE::Robot>(backend) {
}

void EnemyTeam::create_member(unsigned int pattern) {
	members.create(pattern, pattern);
}

XBeeBackend::XBeeBackend(XBeeDongle &dongle, unsigned int camera_mask, int multicast_interface) : Backend(camera_mask, multicast_interface, "10002"), friendly(*this, dongle), enemy(*this) {
}

BackendFactory &XBeeBackend::factory() const {
	return xbee_backend_factory_instance;
}

FriendlyTeam &XBeeBackend::friendly_team() {
	return friendly;
}

const FriendlyTeam &XBeeBackend::friendly_team() const {
	return friendly;
}

EnemyTeam &XBeeBackend::enemy_team() {
	return enemy;
}

const EnemyTeam &XBeeBackend::enemy_team() const {
	return enemy;
}

XBeeBackendFactory::XBeeBackendFactory() : BackendFactory("xbee") {
}

void XBeeBackendFactory::create_backend(const std::string &, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const {
	XBeeDongle dongle;
	dongle.enable();
	XBeeBackend be(dongle, camera_mask, multicast_interface);
	cb(be);
}

