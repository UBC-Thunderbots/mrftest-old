#include "ai/backend/physical/player.h"
#include "ai/backend/ssl_vision/backend.h"
#include "ai/backend/ssl_vision/team.h"
#include "drive/robot.h"
#include "mrf/dongle.h"
#include "xbee/dongle.h"

using namespace AI::BE;

namespace {
	const bool USE_MRF[8] = {
		true, true, true, true,
		true, true, false, false,
	};

	/**
	 * \brief The friendly team.
	 */
	class FriendlyTeam : public AI::BE::SSLVision::Team<AI::BE::Physical::Player, AI::BE::Player> {
		public:
			explicit FriendlyTeam(Backend &backend, XBeeDongle &xbee_dongle, MRFDongle &mrf_dongle);

		protected:
			void create_member(unsigned int pattern);

		private:
			XBeeDongle &xbee_dongle;
			MRFDongle &mrf_dongle;
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
	class HybridBackend : public AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam> {
		public:
			explicit HybridBackend(XBeeDongle &xbee_dongle, MRFDongle &mrf_dongle, unsigned int camera_mask, int multicast_interface);
			BackendFactory &factory() const;
			FriendlyTeam &friendly_team();
			const FriendlyTeam &friendly_team() const;
			EnemyTeam &enemy_team();
			const EnemyTeam &enemy_team() const;

		private:
			FriendlyTeam friendly;
			EnemyTeam enemy;
	};

	class HybridBackendFactory : public BackendFactory {
		public:
			explicit HybridBackendFactory();
			void create_backend(const std::string &, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const;
	};
}

HybridBackendFactory hybrid_backend_factory_instance;

FriendlyTeam::FriendlyTeam(Backend &backend, XBeeDongle &xbee_dongle, MRFDongle &mrf_dongle) : AI::BE::SSLVision::Team<AI::BE::Physical::Player, AI::BE::Player>(backend), xbee_dongle(xbee_dongle), mrf_dongle(mrf_dongle) {
}

void FriendlyTeam::create_member(unsigned int pattern) {
	if (pattern <= 7 && USE_MRF[pattern]) {
		members.create(pattern, pattern, std::ref(mrf_dongle.robot(pattern)));
	} else if (pattern <= 15) {
		members.create(pattern, pattern, std::ref(xbee_dongle.robot(pattern)));
	}
}

EnemyTeam::EnemyTeam(Backend &backend) : AI::BE::SSLVision::Team<AI::BE::Robot, AI::BE::Robot>(backend) {
}

void EnemyTeam::create_member(unsigned int pattern) {
	members.create(pattern, pattern);
}

HybridBackend::HybridBackend(XBeeDongle &xbee_dongle, MRFDongle &mrf_dongle, unsigned int camera_mask, int multicast_interface) : Backend(camera_mask, multicast_interface), friendly(*this, xbee_dongle, mrf_dongle), enemy(*this) {
}

BackendFactory &HybridBackend::factory() const {
	return hybrid_backend_factory_instance;
}

FriendlyTeam &HybridBackend::friendly_team() {
	return friendly;
}

const FriendlyTeam &HybridBackend::friendly_team() const {
	return friendly;
}

EnemyTeam &HybridBackend::enemy_team() {
	return enemy;
}

const EnemyTeam &HybridBackend::enemy_team() const {
	return enemy;
}

HybridBackendFactory::HybridBackendFactory() : BackendFactory("hybrid") {
}

void HybridBackendFactory::create_backend(const std::string &, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const {
	XBeeDongle xbee_dongle;
	xbee_dongle.enable();
	MRFDongle mrf_dongle;
	HybridBackend be(xbee_dongle, mrf_dongle, camera_mask, multicast_interface);
	cb(be);
}

