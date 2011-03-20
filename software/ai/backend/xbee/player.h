#ifndef AI_BACKEND_XBEE_PLAYER_H
#define AI_BACKEND_XBEE_PLAYER_H

#include "ai/backend/xbee/robot.h"
#include "util/annunciator.h"
#include "xbee/robot.h"
#include <ctime>
#include <utility>
#include <vector>

namespace AI {
	namespace BE {
		namespace XBee {
			/**
			 * A player is a robot that can be driven.
			 */
			class Player : public Robot, public AI::BE::Player {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;

					/**
					 * Constructs a new Player object.
					 *
					 * \param[in] backend the backend the player is part of.
					 *
					 * \param[in] pattern the index of the vision pattern associated with the player.
					 *
					 * \param[in] bot the XBee robot being driven.
					 *
					 * \return the new Player.
					 */
					static Ptr create(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot::Ptr bot);

					/**
					 * Drives one tick of time through the RobotController and to the XBee.
					 *
					 * \param[in] halt \c true if the current play type is halt, or \c false if not.
					 */
					void tick(bool halt);

					Visualizable::Colour visualizer_colour() const;
					Glib::ustring visualizer_label() const;
					bool highlight() const { return Robot::highlight(); }
					Visualizable::Colour highlight_colour() const { return Robot::highlight_colour(); }
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
					ObjectStore &object_store() const { return Robot::object_store(); }
					bool has_ball() const;
					bool chicker_ready() const;
					void move_impl(Point dest, double ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio);
					void kick_impl(double speed);
					void autokick_impl(double speed);
					bool has_destination() const { return true; }
					const std::pair<Point, double> &destination() const;
					Point target_velocity() const;
					unsigned int flags() const { return flags_; }
					AI::Flags::MoveType type() const { return move_type_; }
					AI::Flags::MovePrio prio() const { return move_prio_; }
					void path_impl(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) { path_ = p; }
					bool has_path() const { return true; }
					const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const { return path_; }
					void drive(const int(&w)[4]);
					const int(&wheel_speeds() const)[4] {
						return wheel_speeds_;
					}







					/**
					 * Gets the distance the player has travelled while dribbling the ball.
					 *
					 * \return the distance in metres.
					 */
					double dribble_distance() const {
						return dribble_distance_;
					}

				private:
					XBeeRobot::Ptr bot;
					std::pair<Point, double> destination_;
					Point target_velocity_;
					bool moved, controlled;
					double dribble_distance_;
					Point last_dribble_position;
					Annunciator::Message chick_when_not_ready_message;
					int wheel_speeds_[4];
					unsigned int flags_;
					AI::Flags::MoveType move_type_;
					AI::Flags::MovePrio move_prio_;
					std::vector<std::pair<std::pair<Point, double>, timespec> > path_;
					bool autokick_invoked;

					/**
					 * Constructs a new Player object.
					 *
					 * \param[in] backend the backend the player is part of.
					 *
					 * \param[in] name the robot's name.
					 *
					 * \param[in] pattern the index of the vision pattern associated with the player.
					 *
					 * \param[in] bot the XBee robot being driven.
					 */
					Player(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot::Ptr bot);

					/**
					 * Process that makes sure that the dribble motor is not stalled for too long.
					 */
					void dribbler_safety();

					void on_feedback();
			};
		}
	}
}

#endif

