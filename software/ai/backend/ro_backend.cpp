#include "ai/backend/ro_backend.h"
#include "ai/backend/backend.h"
#include "ai/backend/ssl_vision/backend.h"
#include "ai/backend/ssl_vision/team.h"
#include <cstdlib>

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
	 * \brief A player that cannot be controlled, only viewed.
	 */
	class ROPlayer : public AI::BE::Player {
		public:
			typedef BoxPtr<ROPlayer> Ptr;
			ROPlayer(unsigned int pattern);
			void dribble_slow();
			void dribble_stop();
			bool has_ball() const;
			bool chicker_ready() const;
			bool autokick_fired() const;
			void kick_impl(double);
			void autokick_impl(double);
			void chip_impl(double);
			void autochip_impl(double);
			void tick(bool, bool);
	};

	/**
	 * \brief A friendly team in the read-only backend.
	 */
	class FriendlyTeam : public AI::BE::SSLVision::Team<ROPlayer, AI::BE::Player> {
		public:
			FriendlyTeam(AI::BE::Backend &backend);

		protected:
			void create_member(unsigned int pattern);
	};

	/**
	 * \brief An enemy team in the read-only backend.
	 */
	class EnemyTeam : public AI::BE::SSLVision::Team<AI::BE::Robot, AI::BE::Robot> {
		public:
			EnemyTeam(AI::BE::Backend &backend);

		protected:
			void create_member(unsigned int pattern);
	};

	/**
	 * \brief A backend which does not allow driving robots, but only viewing their states on the field.
	 */
	class ROBackend : public AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam> {
		public:
			explicit ROBackend(const std::vector<bool> &disable_cameras, int multicast_interface);
			AI::BE::BackendFactory &factory() const;
			FriendlyTeam &friendly_team();
			const FriendlyTeam &friendly_team() const;
			EnemyTeam &enemy_team();
			const EnemyTeam &enemy_team() const;

		private:
			FriendlyTeam friendly;
			EnemyTeam enemy;
	};

	/**
	 * \brief A factory for creating \ref ROBackend instances.
	 */
	class ROBackendFactory : public AI::BE::BackendFactory {
		public:
			explicit ROBackendFactory();
			void create_backend(const std::vector<bool> &disable_cameras, int multicast_interface, std::function<void(AI::BE::Backend &)> cb) const;
	};
}

ROBackendFactory ro_backend_factory_instance;

ROPlayer::ROPlayer(unsigned int pattern) : AI::BE::Player(pattern) {
}

void ROPlayer::dribble_slow() {
	// Do nothing.
}

void ROPlayer::dribble_stop() {
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

FriendlyTeam::FriendlyTeam(AI::BE::Backend &backend) : AI::BE::SSLVision::Team<ROPlayer, AI::BE::Player>(backend) {
}

void FriendlyTeam::create_member(unsigned int pattern) {
	members[pattern].create(pattern);
}

EnemyTeam::EnemyTeam(AI::BE::Backend &backend) : AI::BE::SSLVision::Team<AI::BE::Robot, AI::BE::Robot>(backend) {
}

void EnemyTeam::create_member(unsigned int pattern) {
	members[pattern].create(pattern);
}

ROBackend::ROBackend(const std::vector<bool> &disable_cameras, int multicast_interface) : Backend(disable_cameras, multicast_interface, vision_port()), friendly(*this), enemy(*this) {
}

AI::BE::BackendFactory &ROBackend::factory() const {
	return ro_backend_factory_instance;
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

void ROBackendFactory::create_backend(const std::vector<bool> &disable_cameras, int multicast_interface, std::function<void(AI::BE::Backend &)> cb) const {
	ROBackend be(disable_cameras, multicast_interface);
	cb(be);
}

AI::BE::BackendFactory &AI::BE::RO::get_factory() {
	return ro_backend_factory_instance;
}

