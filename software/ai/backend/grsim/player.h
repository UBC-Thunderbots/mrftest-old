#ifndef AI_BACKEND_GRSIM_PLAYER_H
#define AI_BACKEND_GRSIM_PLAYER_H

#include "ai/backend/player.h"
#include "proto/grSim_Commands.pb.h"
#include "util/box_ptr.h"

namespace AI {
	namespace BE {
		class Ball;
namespace GRSim { class Player final : public AI::BE::Player {
				public:
					typedef BoxPtr<Player> Ptr;

					explicit Player(unsigned int pattern, const AI::BE::Ball &ball);
					bool has_ball() const override;
					bool chicker_ready() const override;
					bool autokick_fired() const override;
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

					void tick(bool halt, bool stop);
					void encode_orders(grSim_Robot_Command &packet);

				private:
					Point _drive_linear;
					Angle _drive_angular;
					double _drive_kickspeedx;
					double _drive_kickspeedz;
					bool _drive_spinner;

					Point _move_dest;
					Angle _move_ori;
					Angle _pivot_swing;
					double _move_time_delta;
					double _shoot_power;
					double _catch_displacement;
					double _catch_speed;

					Property<Drive::Primitive> _prim;
					int _prim_extra;

					const AI::BE::Ball &_ball;
					bool _autokick_fired;
					bool _had_ball;
					std::chrono::steady_clock::time_point _last_chick_time;
			};
		}
	}
}

inline const Property<Drive::Primitive> &AI::BE::GRSim::Player::primitive() const {
	return _prim;
}

inline void AI::BE::GRSim::Player::move_coast() {	
	_prim = Drive::Primitive::STOP;
	_prim_extra = 0;
}

inline void AI::BE::GRSim::Player::move_brake() {
	_prim = Drive::Primitive::STOP;
	_prim_extra = 1;
}

inline void AI::BE::GRSim::Player::move_move(Point dest) {
	_prim = Drive::Primitive::MOVE;
	_prim_extra = 0;
	_move_time_delta = 0;
	_move_dest = dest;
}

inline void AI::BE::GRSim::Player::move_move(Point dest, Angle orientation) {
	_prim = Drive::Primitive::MOVE;
	_prim_extra = 1;
	_move_time_delta = 0;
	_move_dest = dest;
	_move_ori = orientation;
}

inline void AI::BE::GRSim::Player::move_move(Point dest, double time_delta) {
	_prim = Drive::Primitive::MOVE;
	_prim_extra = 0;
	_move_time_delta = time_delta;
	_move_dest = dest;
}

inline void AI::BE::GRSim::Player::move_move(Point dest, Angle orientation, double time_delta) {
	_prim = Drive::Primitive::MOVE;
	_prim_extra = 1;
	_move_time_delta = time_delta;
	_move_dest = dest;
	_move_ori = orientation;
}

inline void AI::BE::GRSim::Player::move_dribble(Point dest, Angle orientation, bool small_kick_allowed) {
	_prim = Drive::Primitive::DRIBBLE;
	_prim_extra = small_kick_allowed;
	_move_dest = dest;
	_move_ori = orientation;
}

inline void AI::BE::GRSim::Player::move_shoot(Point dest, double power, bool chip) {
	_prim = Drive::Primitive::SHOOT;
	_prim_extra = static_cast<int>(chip);
	_move_dest = dest;
	_shoot_power = power;
}

inline void AI::BE::GRSim::Player::move_shoot(Point dest, Angle orientation, double power, bool chip) {
	_prim = Drive::Primitive::SHOOT;
	_prim_extra = static_cast<int>(chip) | (1 << 1);
	_move_dest = dest;
	_move_ori = orientation;
	_shoot_power = power;
}

inline void AI::BE::GRSim::Player::move_catch(Angle angle_diff, double displacement, double speed) {
	_prim = Drive::Primitive::CATCH;
	_prim_extra = 0;
	_catch_displacement = displacement;
	_catch_speed = speed;
	_move_ori = angle_diff;
}

inline void AI::BE::GRSim::Player::move_pivot(Point centre, Angle swing, Angle orientation) {
	_prim = Drive::Primitive::PIVOT;
	_prim_extra = 0;
	_pivot_swing = swing;
	_move_dest = centre;
	_move_ori = orientation;
}

inline void AI::BE::GRSim::Player::move_spin(Point dest, Angle speed) {
	_prim = Drive::Primitive::SPIN;
	_move_dest = dest;
	_move_ori = speed;
}

#endif
