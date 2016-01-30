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
			bool has_ball() const override;
			double get_lps(unsigned int index) const override;
			bool chicker_ready() const override;
			bool autokick_fired() const override;
			void tick(bool, bool);
			const Property<Drive::Primitive> &primitive() const override;
	
			void move_coast() override;
			void move_brake() override;
			void move_move(Point dest) override;
			void move_move(Point dest, Angle orientation) override;
			void move_move(Point dest, double time_delta) override;
			void move_move(Point dest, Angle orientation, double time_delta) override;
			void move_dribble(Point dest, Angle orientation, double desired_rpm, bool small_kick_allowed) override;
			void move_shoot(Point dest, double power, bool chip) override;
			void move_shoot(Point dest, Angle orientation, double power, bool chip) override;
			void move_catch(Angle angle_diff, double displacement, double speed) override;
			void move_pivot(Point centre, Angle swing, Angle orientation) override;
			void move_spin(Point dest, Angle speed) override;
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
			void log_to(AI::Logger &) override;

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

bool ROPlayer::has_ball() const {
	return false;
}

double ROPlayer::get_lps(unsigned int) const {
	return 0.0;
}

bool ROPlayer::chicker_ready() const {
	return false;
}

bool ROPlayer::autokick_fired() const {
	return false;
}

void ROPlayer::tick(bool, bool) {
	// Do nothing.
}

const Property<Drive::Primitive> &ROPlayer::primitive() const {
	static const Property<Drive::Primitive> prim(Drive::Primitive::STOP);
	return prim;
}

void ROPlayer::move_coast() {	
	// Do nothing.
}

void ROPlayer::move_brake() {
	// Do nothing.
}

void ROPlayer::move_move(Point) {
	// Do nothing.
}

void ROPlayer::move_move(Point, Angle) {
	// Do nothing.
}

void ROPlayer::move_move(Point, double) {
	// Do nothing.
}

void ROPlayer::move_move(Point, Angle, double) {
	// Do nothing.
}

void ROPlayer::move_dribble(Point, Angle, double, bool) {
	// Do nothing.
}

void ROPlayer::move_shoot(Point, double, bool) {
	// Do nothing.
}

void ROPlayer::move_shoot(Point, Angle, double, bool) {
	// Do nothing.
}

void ROPlayer::move_catch(Angle, double, double) {
	// Do nothing.
}

void ROPlayer::move_pivot(Point, Angle, Angle) {
	// Do nothing.
}

void ROPlayer::move_spin(Point, Angle) {
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

void ROBackend::log_to(AI::Logger &) {
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
