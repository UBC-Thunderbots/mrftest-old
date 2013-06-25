#ifndef AI_BACKEND_PHYSICAL_PLAYER_H
#define AI_BACKEND_PHYSICAL_PLAYER_H

#include "ai/backend/player.h"
#include "util/annunciator.h"
#include "util/box_ptr.h"
#include <ctime>
#include <utility>
#include <vector>

namespace Drive {
	class Robot;
}

namespace AI {
	namespace BE {
		namespace Physical {
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
					explicit Player(unsigned int pattern, Drive::Robot &bot);

					/**
					 * \brief Destroys a Player object.
					 */
					~Player();

					/**
					 * \brief Drives one tick of time through the RobotController and to the radio.
					 *
					 * \param[in] halt \c true if the current play type is halt, or \c false if not
					 *
					 * \param[in] stop \c true if the current play type is stop, or \c false if not
					 */
					void tick(bool halt, bool stop);

					bool has_ball() const;
					bool chicker_ready() const;
					void kick_impl(double speed);
					void autokick_impl(double speed);
					void chip_impl(double speed);
					void autochip_impl(double speed);
					bool autokick_fired() const { return autokick_fired_; }
					void dribble_slow();
					unsigned int num_bar_graphs() const;
					double bar_graph_value(unsigned int) const;
					Visualizable::Colour bar_graph_colour(unsigned int) const;

				private:
					Drive::Robot &bot;
					Annunciator::Message robot_dead_message;
					bool autokick_fired_;
					bool dribble_slow_;
					struct AutokickParams {
						bool chip;
						double pulse;
						AutokickParams();
						bool operator==(const AutokickParams &other) const;
						bool operator!=(const AutokickParams &other) const;
					} autokick_params, autokick_params_old;

					void on_autokick_fired();
			};
		}
	}
}

#endif

