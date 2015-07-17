#include "ai/backend/ro_backend.h"
#include "ai/backend/backend.h"
#include "ai/backend/vision/backend.h"
#include "ai/backend/vision/team.h"
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
	 * \brief A player that cannot be controlled, only viewed.
	 */
	class ROPlayer final : public Player {
		public:
			typedef BoxPtr<ROPlayer> Ptr;
			explicit ROPlayer(unsigned int pattern);
			void dribble(DribbleMode mode) override;
			bool has_ball() const override;
			bool chicker_ready() const override;
			bool autokick_fired() const override;
			void kick_impl(double) override;
			void autokick_impl(double) override;
			void chip_impl(double) override;
			void autochip_impl(double) override;
			void tick(bool, bool);
	};

	/**
	 * \brief A friendly team in the read-only backend.
	 */
	class FriendlyTeam final: public AI::BE::Vision::Team<ROPlayer, Player> {
		public:
			explicit FriendlyTeam(Backend &backend);

		protected:
			void create_member(unsigned int pattern) override;
	};

	/**
	 * \brief An enemy team in the read-only backend.
	 */
	class EnemyTeam final: public AI::BE::Vision::Team<Robot, Robot> {
		public:
			explicit EnemyTeam(Backend &backend);

		protected:
			void create_member(unsigned int pattern) override;
	};

	/**
	 * \brief A backend which does not allow driving robots, but only viewing their states on the field.
	 */
	class ROBackend final: public AI::BE::Vision::Backend<FriendlyTeam, EnemyTeam> {
		public:
			explicit ROBackend(const std::vector<bool> &disable_cameras, int multicast_interface);
			BackendFactory &factory() const override;
			FriendlyTeam &friendly_team() override;
			const FriendlyTeam &friendly_team() const override;
			EnemyTeam &enemy_team() override;
			const EnemyTeam &enemy_team() const override;

		private:
			FriendlyTeam friendly;
			EnemyTeam enemy;
	};

	/**
	 * \brief A factory for creating \ref ROBackend instances.
	 */
	class ROBackendFactory final: public BackendFactory {
		public:
			explicit ROBackendFactory();
			std::unique_ptr<Backend> create_backend(const std::vector<bool> &disable_cameras, int multicast_interface) const override;
	};
}

namespace AI {
	namespace BE {
		namespace RO {
			extern ROBackendFactory ro_backend_factory_instance;
		}
	}
}

ROBackendFactory AI::BE::RO::ro_backend_factory_instance;

ROPlayer::ROPlayer(unsigned int pattern) : Player(pattern) {
}

void ROPlayer::dribble(DribbleMode) {
	// Do nothing.
}

bool ROPlayer::has_ball() const {
	return false;
}

bool ROPlayer::chicker_ready() const {
	return false;
}

bool ROPlayer::autokick_fired() const {
	return false;
}

void ROPlayer::kick_impl(double) {
	// Do nothing.
}

void ROPlayer::autokick_impl(double) {
	// Do nothing.
}

void ROPlayer::chip_impl(double) {
	// Do nothing.
}

void ROPlayer::autochip_impl(double) {
	// Do nothing.
}

void ROPlayer::tick(bool, bool) {
	// Do nothing.
}

FriendlyTeam::FriendlyTeam(Backend &backend) : AI::BE::Vision::Team<ROPlayer, Player>(backend) {
}

void FriendlyTeam::create_member(unsigned int pattern) {
	members[pattern].create(pattern);
}

EnemyTeam::EnemyTeam(Backend &backend) : AI::BE::Vision::Team<Robot, Robot>(backend) {
}

void EnemyTeam::create_member(unsigned int pattern) {
	members[pattern].create(pattern);
}

ROBackend::ROBackend(const std::vector<bool> &disable_cameras, int multicast_interface) : Backend(disable_cameras, multicast_interface, vision_port()), friendly(*this), enemy(*this) {
}

BackendFactory &ROBackend::factory() const {
	return AI::BE::RO::ro_backend_factory_instance;
}

FriendlyTeam &ROBackend::friendly_team() {
	return friendly;
}

const FriendlyTeam &ROBackend::friendly_team() const {
	return friendly;
}

EnemyTeam &ROBackend::enemy_team() {
	return enemy;
}

const EnemyTeam &ROBackend::enemy_team() const {
	return enemy;
}

ROBackendFactory::ROBackendFactory() : BackendFactory(u8"ro") {
}

std::unique_ptr<Backend> ROBackendFactory::create_backend(const std::vector<bool> &disable_cameras, int multicast_interface) const {
	std::unique_ptr<Backend> be(new ROBackend(disable_cameras, multicast_interface));
	return be;
}

BackendFactory &AI::BE::RO::get_factory() {
	return ro_backend_factory_instance;
}

