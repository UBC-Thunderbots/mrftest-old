#ifndef AI_BACKEND_PLAYER_H
#define AI_BACKEND_PLAYER_H

#include "ai/flags.h"
#include "ai/backend/robot.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/box_ptr.h"
#include "util/object_store.h"
#include <utility>
#include <vector>

namespace AI {
	namespace BE {
		/**
		 * \brief A player, as exposed by the backend
		 */
		class Player : public AI::BE::Robot {
			public:
				/**
				 * \brief A pointer to a Player
				 */
				typedef BoxPtr<Player> Ptr;

				void move(Point dest, Angle ori, Point vel);
				unsigned int flags() const;
				void flags(unsigned int flags);
				AI::Flags::MoveType type() const;
				void type(AI::Flags::MoveType type);
				AI::Flags::MovePrio prio() const;
				void prio(AI::Flags::MovePrio prio);
				bool has_destination() const;
				std::pair<Point, Angle> destination() const;
				bool has_chipper() const;
				void kick(double speed);
				void autokick(double speed);
				void chip(double power);
				void autochip(double power);
				Point target_velocity() const;
				bool has_path() const;
				const std::vector<std::pair<std::pair<Point, Angle>, timespec>> &path() const;
				void path(const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &p);
				void drive(const int (&w)[4]);
				const int(&wheel_speeds() const)[4];
				void pre_tick();

				Visualizable::Colour visualizer_colour() const;
				bool highlight() const;
				Visualizable::Colour highlight_colour() const;

				virtual bool has_ball() const = 0;
				virtual bool chicker_ready() const = 0;
				virtual bool autokick_fired() const = 0;

			protected:
				bool moved;
				bool controlled;
				int wheel_speeds_[4];

				explicit Player(unsigned int pattern);
				virtual void kick_impl(double speed) = 0;
				virtual void autokick_impl(double speed) = 0;
				virtual void chip_impl(double power) = 0;
				virtual void autochip_impl(double power) = 0;

			private:
				std::pair<Point, Angle> destination_;
				Point target_velocity_;
				unsigned int flags_;
				AI::Flags::MoveType move_type_;
				AI::Flags::MovePrio move_prio_;
				std::vector<std::pair<std::pair<Point, Angle>, timespec>> path_;
		};
	}
}



inline unsigned int AI::BE::Player::flags() const {
	return flags_;
}

inline AI::Flags::MoveType AI::BE::Player::type() const {
	return move_type_;
}

inline void AI::BE::Player::type(AI::Flags::MoveType type) {
	move_type_ = type;
}

inline AI::Flags::MovePrio AI::BE::Player::prio() const {
	return move_prio_;
}

inline void AI::BE::Player::prio(AI::Flags::MovePrio prio) {
	move_prio_ = prio;
}

inline bool AI::BE::Player::has_chipper() const {
	return true;
}

inline void AI::BE::Player::drive(const int (&w)[4]) {
	for (unsigned int i = 0; i < 4; ++i) {
		wheel_speeds_[i] = w[i];
	}
	controlled = true;
}

inline const int(&AI::BE::Player::wheel_speeds() const)[4] {
	return wheel_speeds_;
}

#endif

