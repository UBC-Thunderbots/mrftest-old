#include "ai/backend/physical/player.h"
#include "ai/backend/vision/backend.h"
#include "ai/backend/vision/team.h"
#include "ai/logger.h"
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
			return "10005";
		}
	}

	/**
	 * \brief The friendly team.
	 */
	class FriendlyTeam final : public AI::BE::Vision::Team<AI::BE::Physical::Player, AI::BE::Player> {
		public:
			explicit FriendlyTeam(Backend &backend);
			void log_to(MRFPacketLogger &logger);

		protected:
			void create_member(unsigned int pattern) override;

		private:
			MRFDongle dongle;
	};

	/**
	 * \brief The enemy team.
	 */
	class EnemyTeam final : public AI::BE::Vision::Team<AI::BE::Robot, AI::BE::Robot> {
		public:
			explicit EnemyTeam(Backend &backend);

		protected:
			void create_member(unsigned int pattern) override;
	};

	/**
	 * \brief The backend.
	 */
	class MRFBackend final : public AI::BE::Vision::Backend<FriendlyTeam, EnemyTeam> {
		public:
			explicit MRFBackend(const std::vector<bool> &disable_cameras, int multicast_interface);
			BackendFactory &factory() const override;
			FriendlyTeam &friendly_team() override;
			const FriendlyTeam &friendly_team() const override;
			EnemyTeam &enemy_team() override;
			const EnemyTeam &enemy_team() const override;
			void log_to(AI::Logger &logger) override;

		private:
			FriendlyTeam friendly;
			EnemyTeam enemy;
	};

	class MRFBackendFactory final : public BackendFactory {
		public:
			explicit MRFBackendFactory();
			std::unique_ptr<Backend> create_backend(const std::vector<bool> &disable_cameras, int multicast_interface) const override;
	};
}

MRFBackendFactory mrf_backend_factory_instance;

FriendlyTeam::FriendlyTeam(Backend &backend) : AI::BE::Vision::Team<AI::BE::Physical::Player, AI::BE::Player>(backend) {
}

void FriendlyTeam::log_to(MRFPacketLogger &logger) {
	dongle.log_to(logger);
}

void FriendlyTeam::create_member(unsigned int pattern) {
	if (pattern < 8) {
		members[pattern].create(pattern, std::ref(dongle.robot(pattern)));
	}
}

EnemyTeam::EnemyTeam(Backend &backend) : AI::BE::Vision::Team<AI::BE::Robot, AI::BE::Robot>(backend) {
}

void EnemyTeam::create_member(unsigned int pattern) {
	members[pattern].create(pattern);
}

MRFBackend::MRFBackend(const std::vector<bool> &disable_cameras, int multicast_interface) : Backend(disable_cameras, multicast_interface, vision_port()), friendly(*this), enemy(*this) {
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

void MRFBackend::log_to(AI::Logger &logger) {
	friendly.log_to(logger);
}

MRFBackendFactory::MRFBackendFactory() : BackendFactory(u8"mrf") {
}

std::unique_ptr<Backend> MRFBackendFactory::create_backend(const std::vector<bool> &disable_cameras, int multicast_interface) const {
	std::unique_ptr<Backend> be(new MRFBackend(disable_cameras, multicast_interface));
	return be;
}
