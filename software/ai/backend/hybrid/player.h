#ifndef AI_BACKEND_HYBRID_PLAYER_H
#define AI_BACKEND_HYBRID_PLAYER_H

#include "ai/backend/player.h"
#include "drive/robot.h"
#include "util/annunciator.h"
#include "util/box_ptr.h"
#include <ctime>
#include <utility>
#include <vector>

namespace AI {
	namespace BE {
		namespace Hybrid {
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
					 * \brief Constructs a new Player object.
					 *
					 * \param[in] name the robot's name.
					 *
					 * \param[in] pattern the index of the vision pattern associated with the player.
					 *
					 * \param[in] bot the robot being driven.
					 */
					explicit Player(unsigned int pattern, Drive::Robot *bot);

					/**
					 * \brief Destroys a Player object.
					 */
					~Player();

					/**
					 * \brief Drives one tick of time through the RobotController and to the radio.
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
					Drive::Robot *bot;
					double dribble_distance_;
					Point last_dribble_position;
					int battery_warning_hysteresis;
					Annunciator::Message battery_warning_message;
					bool autokick_fired_;
					struct AutokickParams {
						AutokickParams();
						bool chip;
						double pulse;
						bool operator==(const AutokickParams &other) const;
						bool operator!=(const AutokickParams &other) const;
					};
					AutokickParams autokick_params, autokick_params_old;

					void on_autokick_fired();
			};
		}
	}
}

#endif

