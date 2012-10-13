#ifndef AI_BACKEND_XBEE_PLAYER_H
#define AI_BACKEND_XBEE_PLAYER_H

#include "ai/backend/player.h"
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
			class Player : public AI::BE::Player {
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
					 * \param[in] name the robot's name.
					 *
					 * \param[in] pattern the index of the vision pattern associated with the player.
					 *
					 * \param[in] bot the XBee robot being driven.
					 */
					explicit Player(unsigned int pattern, XBeeRobot &bot);

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

					bool has_ball() const;
					bool chicker_ready() const;
					void kick_impl(double speed);
					void autokick_impl(double speed);
					void chip_impl(double speed);
					void autochip_impl(double speed);
					bool autokick_fired() const { return autokick_fired_; }
					unsigned int num_bar_graphs() const;
					double bar_graph_value(unsigned int) const;
					Visualizable::Colour bar_graph_colour(unsigned int) const;







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
					double dribble_distance_;
					Point last_dribble_position;
					int battery_warning_hysteresis;
					Annunciator::Message battery_warning_message;
					bool autokick_invoked;
					bool autokick_fired_;

					void on_autokick_fired();
			};
		}
	}
}

#endif

