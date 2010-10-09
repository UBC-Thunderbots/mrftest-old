#ifndef AI_BACKEND_XBEED_PLAYER_H
#define AI_BACKEND_XBEED_PLAYER_H

#include "ai/backend/xbeed/robot.h"
#include "util/annunciator.h"
#include "xbee/client/drive.h"
#include <ctime>
#include <utility>
#include <vector>

namespace AI {
	namespace BE {
		namespace XBeeD {
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
					static Ptr create(AI::BE::Backend &backend, unsigned int pattern, XBeeDriveBot::Ptr bot);

					/**
					 * Drives one tick of time through the RobotController and to the XBee.
					 *
					 * \param[in] scram whether or not to scram the robot.
					 */
					void tick(bool scram);

					Visualizable::RobotColour visualizer_colour() const;
					Glib::ustring visualizer_label() const;
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
					bool has_ball() const { return sense_ball(); }
					unsigned int chicker_ready_time() const;
					void move(Point dest, double ori, unsigned int flags, AI::Flags::MOVE_TYPE type, AI::Flags::MOVE_PRIO prio);
					void kick(double power);
					void chip(double power);
					const std::pair<Point, double> &destination() const;
					unsigned int flags() const { return flags_; }
					AI::Flags::MOVE_TYPE type() const { return move_type_; }
					AI::Flags::MOVE_PRIO prio() const { return move_prio_; }
					void path(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) { path_ = p; }
					const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const { return path_; }
					void drive(const int(&w)[4]);
					const int(&wheel_speeds() const)[4] {
						return wheel_speeds_;
					}







					/**
					 * Sets the speed of the dribbler motor.
					 * If this is not invoked by the %AI in a particular time tick, the dribbler will turn off.
					 *
					 * \param[in] speed the speed to run at, from 0 to 1.
					 */
					void dribble(double speed);

					/**
					 * Gets the number of times the dribbler motor has sensed load proportional to having the ball.
					 * This can have false positives, especially if the dribbler is spinning up or down.
					 *
					 * \return the number of consecutive times the robot's dribbler sensed the ball.
					 */
					int sense_ball() const {
						return sense_ball_;
					}

					/**
					 * Gets the number of seconds the dribbler motor has sensed load proportional to having the ball.
					 * This can have false positives, especially if the dribbler is spinning up or down.
					 *
					 * \return the number of seconds for which the robot's dribbler sensed the ball.
					 */
					double sense_ball_time() const;

					/**
					 * Gets the number of seconds since the player stopped sensing the ball on its dribbler.
					 * This can be used to detect a momentary loss of sensing as compared to a complete genuine loss of ball.
					 *
					 * \return the number of seconds elapsed since the player last sensed the ball.
					 */
					double last_sense_ball_time() const;

					/**
					 * Gets the speed the dribbler would be spinning at given the current power level if it were unloaded and had finished spinning up.
					 *
					 * \return the theoretical dribbler speed, in RPM.
					 */
					unsigned int theory_dribbler_speed() const {
						return theory_dribble_rpm;
					}

					/**
					 * Gets the current speed the dribbler is spinning.
					 *
					 * \return the dribbler speed, in RPM.
					 */
					unsigned int dribbler_speed() const {
						return bot->dribbler_speed();
					}

					/**
					 * Gets the distance the player has travelled while dribbling the ball.
					 *
					 * \return the distance in metres.
					 */
					double dribble_distance() const {
						return dribble_distance_;
					}

					/**
					 * Checks whether the dribbler is currently safe to use (i.e. isn't too hot from stallling a lot).
					 *
					 * \return \c true if the dribbler can be used, or \c false if not.
					 */
					bool dribbler_safe() const;

					/**
					 * The value returned by chicker_ready_time() const if the chicker is probably never going to be ready.
					 */
					static const unsigned int CHICKER_FOREVER;

				private:
					XBeeDriveBot::Ptr bot;
					std::pair<Point, double> destination_;
					bool moved, controlled;
					int new_dribble_power;
					int old_dribble_power;
					int sense_ball_;
					bool dribble_stall;
					unsigned int theory_dribble_rpm;
					timespec sense_ball_start, sense_ball_end, stall_start, recover_time_start, chicker_last_fire_time;
					double dribble_distance_;
					Point last_dribble_position;
					Annunciator::Message not_moved_message, chick_when_not_ready_message;
					int wheel_speeds_[4];
					unsigned int flags_;
					AI::Flags::MOVE_TYPE move_type_;
					AI::Flags::MOVE_PRIO move_prio_;
					std::vector<std::pair<std::pair<Point, double>, timespec> > path_;

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
					Player(AI::BE::Backend &backend, unsigned int pattern, XBeeDriveBot::Ptr bot);

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

