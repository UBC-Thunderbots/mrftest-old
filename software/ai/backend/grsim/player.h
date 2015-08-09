#ifndef AI_BACKEND_GRSIM_PLAYER_H
#define AI_BACKEND_GRSIM_PLAYER_H

#include "ai/backend/player.h"
#include "proto/grSim_Commands.pb.h"
#include "util/box_ptr.h"

namespace AI {
	namespace BE {
		class Ball;

		namespace GRSim {
			class Player final : public AI::BE::Player {
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
					enum class ChickMode {
						IDLE,
						KICK,
						CHIP,
						AUTOKICK,
						AUTOCHIP,
					};

					const AI::BE::Ball &ball;
					bool dribble_requested, dribble_active;
					bool autokick_fired_;
					bool had_ball;
					ChickMode chick_mode;
					double chick_power;
					std::chrono::steady_clock::time_point last_chick_time;
			};
		}
	}
}

#warning movement primitives arent yet implemented in grsim wrapper
inline const Property<Drive::Primitive> &AI::BE::GRSim::Player::primitive() const {
	static Property<Drive::Primitive> prim(Drive::Primitive::STOP);
	return prim;
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_coast() {	
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_brake() {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_move(Point dest) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_move(Point dest, Angle orientation) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_move(Point dest, double time_delta) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_move(Point dest, Angle orientation, double time_delta) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_dribble(Point dest, Angle orientation, bool small_kick_allowed) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_shoot(Point dest, double power, bool chip) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_shoot(Point dest, Angle orientation, double power, bool chip) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_catch(Angle angle_diff, double displacement, double speed) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_pivot(Point centre, Angle swing, Angle orientation) {
}

#warning movement primitives arent yet implemented in grsim wrapper
inline void AI::BE::GRSim::Player::move_spin(Point dest, Angle speed) {
}

#endif

