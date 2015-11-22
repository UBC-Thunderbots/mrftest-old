#ifndef AI_BACKEND_PLAYER_H
#define AI_BACKEND_PLAYER_H

#include "ai/flags.h"
#include "ai/backend/robot.h"
#include "ai/common/objects/time.h"
#include "drive/robot.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/box_ptr.h"
#include "util/object_store.h"
#include <algorithm>
#include <utility>
#include <vector>

namespace AI {
	struct PrimitiveInfo {
		Drive::Primitive type;
		//int type;
		Point field_point;
		Angle field_angle;
		Angle field_angle2;
		double field_double;
		bool field_bool;
		bool care_angle;
	};

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

				/**
				 * \brief The possible ways a robot can dribble.
				 */
				enum class DribbleMode {
					/**
					 * \brief The dribbler is off.
					 */
					STOP,

					/**
					 * \brief The dribbler is running to catch an incoming ball.
					 */
					CATCH,

					/**
					 * \brief The dribbler is running to pick up a loose ball.
					 */
					INTERCEPT,

					/**
					 * \brief The dribbler is running to carry a possessed ball.
					 */
					CARRY,
				};

				void move(Point dest, Angle ori, Point vel);
				unsigned int flags() const;
				void flags(unsigned int flags);
				AI::Flags::MoveType type() const;
				void type(AI::Flags::MoveType type);
				AI::Flags::MovePrio prio() const;
				void prio(AI::Flags::MovePrio prio);
				bool has_destination() const final override;
				std::pair<Point, Angle> destination() const final override;

				void destination(std::pair<Point, Angle> dest);
				virtual const Property<Drive::Primitive> &primitive() const = 0;
				virtual void move_coast() = 0;
				virtual void move_brake() = 0;
				virtual void move_move(Point dest) = 0;
				virtual void move_move(Point dest, Angle orientation) = 0;
				virtual void move_move(Point dest, double time_delta) = 0;
				virtual void move_move(Point dest, Angle orientation, double time_delta) = 0; 
				virtual void move_dribble(Point dest, Angle orientation, bool small_kick_allowed) = 0;
				virtual void move_shoot(Point dest, double power, bool chip) = 0;
				virtual void move_shoot(Point dest, Angle orientation, double power, bool chip) = 0;
				virtual void move_catch(Angle angle_diff, double displacement, double speed) = 0;
				virtual void move_pivot(Point centre, Angle swing, Angle orientation) = 0;
				virtual void move_spin(Point dest, Angle speed) = 0;
				Point target_velocity() const;
				bool has_display_path() const final override;
				const std::vector<Point> &display_path() const final override;
				void display_path(const std::vector<Point> &p);
				void pre_tick();
				void update_predictor(AI::Timestamp ts);	

				Visualizable::Colour visualizer_colour() const final override;
				bool highlight() const final override;
				Visualizable::Colour highlight_colour() const final override;

				virtual bool has_ball() const = 0;
				virtual double get_lps(unsigned int index) const = 0;
				virtual bool chicker_ready() const = 0;
				virtual bool autokick_fired() const = 0;

				/*struct PrimitiveInfo {
					Drive::Primitive type;
					//int type;
					Point field_point;
					Angle field_angle;
					double field_double;
					bool field_bool;
					bool care_angle;
				};*/
				AI::PrimitiveInfo hl_request;

			protected:
				bool moved;

				explicit Player(unsigned int pattern);

			private:
				std::pair<Point, Angle> destination_;
				Point target_velocity_;
				unsigned int flags_;
				AI::Flags::MoveType move_type_;
				AI::Flags::MovePrio move_prio_;
				std::vector<Point> display_path_;
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

#endif

