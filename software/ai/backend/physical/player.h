#ifndef AI_BACKEND_PHYSICAL_PLAYER_H
#define AI_BACKEND_PHYSICAL_PLAYER_H

#include "ai/backend/player.h"
#include "util/annunciator.h"
#include "util/box_ptr.h"
#include "drive/robot.h"
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
			class Player final : public AI::BE::Player {
				public:
					/**
					 * \brief A pointer to a Player.
					 */
					typedef BoxPtr<Player> Ptr;

					/**
					 * \brief Constructs a new Player object.
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

					bool has_ball() const override;
					bool chicker_ready() const override;
					const Property<Drive::Primitive> &primitive() const override;

					void move_coast() override;
					void move_brake() override;
					void move_move(Point dest) override;
					void move_move(Point dest, Angle orientation) override;
					void move_move(Point dest, double time_delta) override;
					void move_move(Point dest, Angle orientation, double time_delta) override;
					void move_dribble(Point dest, Angle orientation, bool small_kick_allowed) override;
					void move_shoot(Point dest, double power, bool chip) override;
					void move_shoot(Point dest, Angle orientation, double power, bool chip) override;
					void move_catch(Angle angle_diff, double displacement, double speed) override;
					void move_pivot(Point centre, Angle swing, Angle orientation) override;
					void move_spin(Point dest, Angle speed) override;
				
					bool autokick_fired() const override { return autokick_fired_; }
					unsigned int num_bar_graphs() const override;
					double get_lps(unsigned int index) const override;
					double bar_graph_value(unsigned int) const override;
					Visualizable::Colour bar_graph_colour(unsigned int) const override;

				private:
					Drive::Robot &bot;
					Annunciator::Message robot_dead_message;
					bool autokick_fired_;

					void on_autokick_fired();
			};

		}
	}
}



inline const Property<Drive::Primitive> &AI::BE::Physical::Player::primitive() const {
	return bot.primitive;
}

inline void AI::BE::Physical::Player::move_coast() {
	bot.move_coast();
}

inline void AI::BE::Physical::Player::move_brake() {
	bot.move_brake();
}

inline void AI::BE::Physical::Player::move_move(Point dest) {
	bot.move_move(dest);
}

inline void AI::BE::Physical::Player::move_move(Point dest, Angle orientation) {
	bot.move_move(dest, orientation);
}

inline void AI::BE::Physical::Player::move_move(Point dest, double time_delta) {
	bot.move_move(dest, time_delta);
}

inline void AI::BE::Physical::Player::move_move(Point dest, Angle orientation, double time_delta) {
	bot.move_move(dest, orientation, time_delta);
}

inline void AI::BE::Physical::Player::move_dribble(Point dest, Angle orientation, bool small_kick_allowed) {
	bot.move_dribble(dest, orientation, small_kick_allowed);
}

inline void AI::BE::Physical::Player::move_shoot(Point dest, double power, bool chip) {
	bot.move_shoot(dest, power, chip);
}

inline void AI::BE::Physical::Player::move_shoot(Point dest, Angle orientation, double power, bool chip) {
	bot.move_shoot(dest, orientation, power, chip);
}

inline void AI::BE::Physical::Player::move_catch(Angle angle_diff, double displacement, double speed) {
	bot.move_catch(angle_diff, displacement, speed);
}

inline void AI::BE::Physical::Player::move_pivot(Point centre, Angle swing, Angle orientation) {
	bot.move_pivot(centre, swing, orientation);
}

inline void AI::BE::Physical::Player::move_spin(Point dest, Angle speed) {
	bot.move_spin(dest, speed);
}

#endif
