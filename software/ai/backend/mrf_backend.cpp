#include "ai/backend/physical/player.h"
#include "ai/backend/vision/backend.h"
#include "ai/backend/vision/team.h"
#include "ai/logger.h"
#include "mrf/dongle.h"
#include "mrf/robot.h"
#include <cstdlib>
#include <tuple>
#include <vector>
#include <algorithm>
#include <iostream>

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
			void update(const std::vector<const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *> &packets, const std::vector<AI::Timestamp> &ts);

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


void FriendlyTeam::update(const std::vector<const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *> &packets, const std::vector<AI::Timestamp> &ts) {

	if(packets.empty()) return;

	bool membership_changed = false;

	std::size_t newest_index = 0;
	AI::Timestamp max_time = ts[0];
	for(std::size_t i = 1; i < packets.size(); i++){
		if(ts[i] >= max_time){
			max_time = ts[i];
			newest_index = i;
		}
	}

	std::vector<std::tuple<uint8_t,Point, Angle>> newdetbots;

	// Update existing robots and create new robots.
	bool seen_this_frame[NUM_PATTERNS];
	std::fill_n(seen_this_frame, NUM_PATTERNS, false);
	for (std::size_t i = 0; i < packets.size(); ++i) {
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(*packets[i]);
		for (std::size_t j = 0; j < static_cast<std::size_t>(rep.size()); ++j) {
			const SSL_DetectionRobot &detbot = rep.Get(static_cast<int>(j));
			if (detbot.has_robot_id()) {
				unsigned int pattern = detbot.robot_id();
				if (pattern < NUM_PATTERNS) {
					const AI::BE::Physical::Player::Ptr &bot = members[pattern].ptr();
					if (!bot) {
						create_member(pattern);
						membership_changed = true;
					}
					if (bot && !seen_this_frame[bot->pattern()]) {
						seen_this_frame[bot->pattern()] = true;
						if (detbot.has_orientation()) {
							bool neg = backend.defending_end() == AI::BE::Backend::FieldEnd::EAST;
							Point pos((neg ? -detbot.x() : detbot.x()) / 1000.0, (neg ? -detbot.y() : detbot.y()) / 1000.0);
							Angle ori = (Angle::of_radians(detbot.orientation()) + (neg ? Angle::half() : Angle::zero())).angle_mod();
							bot->add_field_data(pos, ori, ts[i]);
							if(i == newest_index){
								newdetbots.push_back(std::make_tuple(pattern, pos, ori));
							}

						} else {
							LOG_WARN(u8"Vision packet has robot with no orientation.");
						}
					}
				}
			}
		}
	}

	// Count failures.
	for (Box<AI::BE::Physical::Player> &i : members) {
		if (i) {
			const AI::BE::Physical::Player::Ptr &bot = i.ptr();
			assert(bot->pattern() < NUM_PATTERNS);
			if (!seen_this_frame[bot->pattern()]) {
				++vision_failures[bot->pattern()];
			} else {
				vision_failures[bot->pattern()] = 0;
			}
			seen_this_frame[bot->pattern()] = false;
			if (vision_failures[bot->pattern()] >= Vision::MAX_VISION_FAILURES) {
				i.destroy();
				membership_changed = true;
			}
		}
	}

	// If membership changed, rebuild the pointer array and emit the signal.
	if (membership_changed) {
		populate_pointers();
		AI::BE::Team<AI::BE::Player>::signal_membership_changed().emit();
	}

	if(newdetbots.empty()) return;
	else{
		uint64_t int_time = max_time.time_since_epoch().count();

		std::cout << "Calling dongle.send_camera_packet with: ";
		for (std::size_t i = 0; i < newdetbots.size(); ++i) {
			std::cout << "bot number = " << unsigned(std::get<0>(newdetbots[i])) << ", ";
			std::cout << "x = " << (std::get<1>(newdetbots[i])).x << ", ";
			std::cout << "y = " << (std::get<1>(newdetbots[i])).y << ", ";
			std::cout << "theta = " << (std::get<2>(newdetbots[i])).to_degrees() << std::endl;
		}

		dongle.send_camera_packet(newdetbots, Point(-7,-7), &int_time);
	}
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
