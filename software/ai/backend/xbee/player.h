#ifndef AI_BACKEND_XBEE_PLAYER_H
#define AI_BACKEND_XBEE_PLAYER_H

#include "ai/backend/xbee/robot.h"
#include "util/annunciator.h"
#include "util/box_ptr.h"
#include "xbee/robot.h"
#include <ctime>
#include <utility>
#include <vector>

namespace AI {
	namespace BE {
		namespace XBee {
			/**
			 * \brief A player is a robot that can be driven.
			 */
			class Player : public Robot, public AI::BE::Player {
				public:
					/**
					 * \brief A pointer to a Player.
					 */
					typedef BoxPtr<Player> Ptr;

					/**
					 * \brief A pointer to a const Player.
					 */
					typedef BoxPtr<const Player> CPtr;

					/**
					 * \brief Constructs a new Player object.
					 *
					 * \param[in] backend the backend the player is part of.
					 *
					 * \param[in] name the robot's name.
					 *
					 * \param[in] pattern the index of the vision pattern associated with the player.
					 *
					 * \param[in] bot the XBee robot being driven.
					 */
					explicit Player(AI::BE::Backend &backend, unsigned int pattern, XBeeRobot &bot);

					/**
					 * \brief Destroys a Player object.
					 */
					~Player();

					/**
					 * \brief Drives one tick of time through the RobotController and to the XBee.
					 *
					 * \param[in] halt \c true if the current play type is halt, or \c false if not.
					 */
					void tick(bool halt);

					Visualizable::Colour visualizer_colour() const;
					Glib::ustring visualizer_label() const;
					bool highlight() const;
					Visualizable::Colour highlight_colour() const;
					Point position(double delta = 0.0) const { return AI::BE::XBee::Robot::position(delta); }
					Point velocity(double delta = 0.0) const { return AI::BE::XBee::Robot::velocity(delta); }
					Angle orientation(double delta = 0.0) const { return AI::BE::XBee::Robot::orientation(delta); }
					Angle avelocity(double delta = 0.0) const { return AI::BE::XBee::Robot::avelocity(delta); }
					Point position_stdev(double delta = 0.0) const { return AI::BE::XBee::Robot::position_stdev(delta); }
					Point velocity_stdev(double delta = 0.0) const { return AI::BE::XBee::Robot::velocity_stdev(delta); }
					Angle orientation_stdev(double delta = 0.0) const { return AI::BE::XBee::Robot::orientation_stdev(delta); }
					Angle avelocity_stdev(double delta = 0.0) const { return AI::BE::XBee::Robot::avelocity_stdev(delta); }
					unsigned int pattern() const { return AI::BE::XBee::Robot::pattern(); }
					ObjectStore &object_store() const { return AI::BE::XBee::Robot::object_store(); }
					bool alive() const;
					bool has_ball() const;
					bool chicker_ready() const;
					void kick_impl(double speed);
					void autokick_impl(double speed);
					void chip_impl(double speed);
					void autochip_impl(double speed);
					bool autokick_fired() const { return autokick_fired_; }
					bool has_destination() const { return true; }
					const std::pair<Point, Angle> &destination() const;
					Point target_velocity() const;
					void path_impl(const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &p) { path_ = p; }
					bool has_path() const { return true; }
					const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &path() const { return path_; }
					using AI::BE::Player::path;
					unsigned int num_bar_graphs() const;
					double bar_graph_value(unsigned int) const;
					Visualizable::Colour bar_graph_colour(unsigned int) const;
					void drive(const int(&w)[4]);
					const int(&wheel_speeds() const)[4] { return wheel_speeds_; }
					void avoid_distance(AI::Flags::AvoidDistance dist) const { AI::BE::XBee::Robot::avoid_distance(dist); }
					AI::Flags::AvoidDistance avoid_distance() const { return AI::BE::XBee::Robot::avoid_distance(); }
					void pre_tick() { AI::BE::Player::pre_tick(); }







					/**
					 * \brief Gets the distance the player has travelled while dribbling the ball.
					 *
					 * \return the distance in metres.
					 */
					double dribble_distance() const {
						return dribble_distance_;
					}

				private:
					XBeeRobot &bot;
					bool controlled;
					double dribble_distance_;
					Point last_dribble_position;
					int battery_warning_hysteresis;
					Annunciator::Message battery_warning_message;
					int wheel_speeds_[4];
					std::vector<std::pair<std::pair<Point, Angle>, timespec> > path_;
					bool autokick_invoked;
					bool autokick_fired_;

					void on_autokick_fired();
			};
		}
	}
}

#endif

