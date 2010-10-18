#ifndef AI_BACKEND_SIMULATOR_PLAYER_H
#define AI_BACKEND_SIMULATOR_PLAYER_H

#include "ai/backend/backend.h"
#include "ai/backend/simulator/robot.h"
#include "simulator/sockproto/proto.h"
#include <algorithm>
#include <sigc++/sigc++.h>

namespace AI {
	namespace BE {
		namespace Simulator {
			/**
			 * A friendly robot that exists in a simulator.
			 */
			class Player : public AI::BE::Simulator::Robot, public AI::BE::Player {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;

					/**
					 * Constructs a new Player.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 *
					 * \return the new Player.
					 */
					static Ptr create(unsigned int pattern) {
						Ptr p(new Player(pattern));
						return p;
					}

					/**
					 * Updates the state of the player and locks in its predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] ts the timestamp at which the robot was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2APlayerInfo &state, const timespec &ts) {
						AI::BE::Simulator::Robot::pre_tick(state.robot_info, ts);
						has_ball_ = state.has_ball;
						kick_ = chip_ = false;
					}

					/**
					 * Encodes the robot's current orders into a packet for transmission to the simulator.
					 *
					 * \param[out] orders the packet to encode into.
					 */
					void encode_orders(::Simulator::Proto::A2SPlayerInfo &orders) {
						orders.pattern = pattern();
						orders.kick = kick_;
						orders.chip = chip_;
						orders.chick_power = chick_power_;
						std::copy(&wheel_speeds_[0], &wheel_speeds_[4], &orders.wheel_speeds[0]);
					}

					Visualizable::RobotColour visualizer_colour() const { return Visualizable::RobotColour(0.0, 1.0, 0.0); }
					Glib::ustring visualizer_label() const { return Robot::visualizer_label(); }
					Point position() const { return Robot::position(); }
					Point position(double delta) const { return Robot::position(delta); }
					Point position(const timespec &ts) const { return Robot::position(ts); }
					Point velocity(double delta = 0.0) const { return Robot::velocity(delta); }
					Point velocity(const timespec &ts) const { return Robot::velocity(ts); }
					Point acceleration(double delta = 0.0) const { return Robot::acceleration(delta); }
					Point acceleration(const timespec &ts) const { return Robot::acceleration(ts); }
					double orientation() const { return Robot::orientation(); }
					double orientation(double delta) const { return Robot::orientation(delta); }
					double orientation(const timespec &ts) const { return Robot::orientation(ts); }
					double avelocity(double delta = 0.0) const { return Robot::avelocity(delta); }
					double avelocity(const timespec &ts) const { return Robot::avelocity(ts); }
					double aacceleration(double delta = 0.0) const { return Robot::aacceleration(delta); }
					double aacceleration(const timespec &ts) const { return Robot::aacceleration(ts); }
					unsigned int pattern() const { return Robot::pattern(); }
					ObjectStore &object_store() { return Robot::object_store(); }
					bool has_ball() const { return has_ball_; }
					unsigned int chicker_ready_time() const { return 0; }
					void move(Point dest, double ori, unsigned int flags, AI::Flags::MOVE_TYPE type, AI::Flags::MOVE_PRIO prio) {
						destination_.first = dest;
						destination_.second = ori;
						flags_ = flags;
						move_type_ = type;
						move_prio_ = prio;
					}
					void kick(double power) { kick_ = true; chick_power_ = power; }
					void chip(double power) { chip_ = true; chick_power_ = power; }
					const std::pair<Point, double> &destination() const { return destination_; }
					unsigned int flags() const { return flags_; }
					AI::Flags::MOVE_TYPE type() const { return move_type_; }
					AI::Flags::MOVE_PRIO prio() const { return move_prio_; }
					void path(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) { path_ = p; }
					const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const { return path_; }
					void drive(const int(&w)[4]) { std::copy(&w[0], &w[4], &wheel_speeds_[0]); }
					const int(&wheel_speeds() const)[4] { return wheel_speeds_; }

				protected:
					/**
					 * Constructs a new Player.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 */
					Player(unsigned int pattern) : AI::BE::Simulator::Robot(pattern), has_ball_(false), flags_(0), move_type_(AI::Flags::MOVE_NORMAL), move_prio_(AI::Flags::PRIO_LOW), kick_(false), chip_(false), chick_power_(0.0) {
						std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
					}

					/**
					 * Destroys the Player.
					 */
					~Player() {
					}

				private:
					/**
					 * Whether or not this player is holding the ball on its dribbler.
					 */
					bool has_ball_;

					/**
					 * The target position and orientation most recently selected by the high-level AI.
					 */
					std::pair<Point, double> destination_;

					/**
					 * The movement flags most recently specified by the high-level AI.
					 */
					unsigned int flags_;

					/**
					 * The movement type most recently specified by the high-level AI.
					 */
					AI::Flags::MOVE_TYPE move_type_;

					/**
					 * The movement priority most recently specified by the high-level AI.
					 */
					AI::Flags::MOVE_PRIO move_prio_;

					/**
					 * The path of positions, orientations, and deadline timestamps most recently specified by the navigator.
					 */
					std::vector<std::pair<std::pair<Point, double>, timespec> > path_;

					/**
					 * Whether the AI elected to kick in the current time tick.
					 */
					bool kick_;

					/**
					 * Whether the AI elected to chip in the current time tick.
					 */
					bool chip_;

					/**
					 * The power level for the kick or chip, if one of the flags is set.
					 */
					double chick_power_;

					/**
					 * The rotational speeds requested for the wheels by the robot controller,
					 * in quarters of a degree of motor shaft rotation per five milliseconds.
					 */
					int wheel_speeds_[4];
			};
		}
	}
}

#endif

