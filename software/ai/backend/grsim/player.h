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
					void dribble(DribbleMode mode) override;
					bool has_ball() const override;
					bool chicker_ready() const override;
					bool autokick_fired() const override;
					void tick(bool halt, bool stop);
					void encode_orders(grSim_Robot_Command &packet);

				protected:
					void kick_impl(double speed) override;
					void autokick_impl(double speed) override;
					void chip_impl(double power) override;
					void autochip_impl(double power) override;

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

#endif

