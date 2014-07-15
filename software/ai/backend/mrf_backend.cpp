#include "ai/backend/physical/player.h"
#include "ai/backend/ssl_vision/backend.h"
#include "ai/backend/ssl_vision/team.h"
#include "mrf/dongle.h"
#include "mrf/robot.h"
#include <cstdlib>

using namespace AI::BE;

namespace {
	/**
	 * \brief Returns the port number to use for SSL-Vision data.
	 *
	 * \return the port number, as a string
	 */
	const char *vision_port() {
		const char *evar = std::getenv("SSL_VISION_PORT");
		if (evar) {
			return evar;
		} else {
			return "10002";
		}
	}

	/**
	 * \brief The friendly team.
	 */
	class FriendlyTeam : public AI::BE::SSLVision::Team<AI::BE::Physical::Player, AI::BE::Player> {
		public:
			explicit FriendlyTeam(Backend &backend, MRFDongle &dongle);

		protected:
			void create_member(unsigned int pattern);

		private:
			MRFDongle &dongle;
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
	class MRFBackend : public AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam> {
		public:
			explicit MRFBackend(const std::vector<bool> &disable_cameras, MRFDongle &dongle, int multicast_interface);
			BackendFactory &factory() const;
			FriendlyTeam &friendly_team();
			const FriendlyTeam &friendly_team() const;
			EnemyTeam &enemy_team();
			const EnemyTeam &enemy_team() const;

		private:
			FriendlyTeam friendly;
			EnemyTeam enemy;
	};

	class MRFBackendFactory : public BackendFactory {
		public:
			explicit MRFBackendFactory();
			void create_backend(const std::vector<bool> &disable_cameras, int multicast_interface, std::function<void(Backend &)> cb) const;
	};
}

MRFBackendFactory mrf_backend_factory_instance;

FriendlyTeam::FriendlyTeam(Backend &backend, MRFDongle &dongle) : AI::BE::SSLVision::Team<AI::BE::Physical::Player, AI::BE::Player>(backend), dongle(dongle) {
}

void FriendlyTeam::create_member(unsigned int pattern) {
	if (pattern < 8) {
		members[pattern].create(pattern, std::ref(dongle.robot(pattern)));
	}
}

EnemyTeam::EnemyTeam(Backend &backend) : AI::BE::SSLVision::Team<AI::BE::Robot, AI::BE::Robot>(backend) {
}

void EnemyTeam::create_member(unsigned int pattern) {
	members[pattern].create(pattern);
}

MRFBackend::MRFBackend(const std::vector<bool> &disable_cameras, MRFDongle &dongle, int multicast_interface) : Backend(disable_cameras, multicast_interface, vision_port()), friendly(*this, dongle), enemy(*this) {
}

BackendFactory &MRFBackend::factory() const {
	return mrf_backend_factory_instance;
}

FriendlyTeam &MRFBackend::friendly_team() {
	return friendly;
}

const FriendlyTeam &MRFBackend::friendly_team() const {
	return friendly;
}

EnemyTeam &MRFBackend::enemy_team() {
	return enemy;
}

const EnemyTeam &MRFBackend::enemy_team() const {
	return enemy;
}

MRFBackendFactory::MRFBackendFactory() : BackendFactory(u8"mrf") {
}

void MRFBackendFactory::create_backend(const std::vector<bool> &disable_cameras, int multicast_interface, std::function<void(Backend &)> cb) const {
	MRFDongle dongle;
	MRFBackend be(disable_cameras, dongle, multicast_interface);
	cb(be);
}

