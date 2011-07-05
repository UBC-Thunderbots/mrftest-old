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
					bool highlight() const;
					Visualizable::Colour highlight_colour() const;
					Point position(double delta = 0.0) const { return Robot::position(delta); }
					Point velocity(double delta = 0.0) const { return Robot::velocity(delta); }
					double orientation(double delta = 0.0) const { return Robot::orientation(delta); }
					double avelocity(double delta = 0.0) const { return Robot::avelocity(delta); }
					Point position_stdev(double delta = 0.0) const { return Robot::position_stdev(delta); }
					Point velocity_stdev(double delta = 0.0) const { return Robot::velocity_stdev(delta); }
					double orientation_stdev(double delta = 0.0) const { return Robot::orientation_stdev(delta); }
					double avelocity_stdev(double delta = 0.0) const { return Robot::avelocity_stdev(delta); }
					unsigned int pattern() const { return Robot::pattern(); }
					ObjectStore &object_store() const { return Robot::object_store(); }
					bool has_ball() const;
					bool chicker_ready() const;
					bool kicker_directional() const;
					void kick_impl(double speed, double angle);
					void autokick_impl(double speed, double angle);
					bool autokick_fired() const { return autokick_fired_; }
					bool has_destination() const { return true; }
					const std::pair<Point, double> &destination() const;
					Point target_velocity() const;
					void path_impl(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) { path_ = p; }
					bool has_path() const { return true; }
					const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const { return path_; }
					void drive(const int(&w)[4]);
					const int(&wheel_speeds() const)[4] {
						return wheel_speeds_;
					}
					void avoid_distance(AI::Flags::AvoidDistance dist) { Robot::avoid_distance(dist); }
					AI::Flags::AvoidDistance avoid_distance() const { return Robot::avoid_distance(); }
					void pre_tick() { AI::BE::Player::pre_tick(); }







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
					bool controlled;
					double dribble_distance_;
					Point last_dribble_position;
					int battery_warning_hysteresis;
					Annunciator::Message battery_warning_message;
					int wheel_speeds_[4];
					std::vector<std::pair<std::pair<Point, double>, timespec> > path_;
					bool autokick_invoked;
					bool kicker_directional_;
					bool autokick_fired_;

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
					 * \brief Destroys a Player object.
					 */
					~Player();

					void on_autokick_fired();
			};
		}
	}
}

#endif

