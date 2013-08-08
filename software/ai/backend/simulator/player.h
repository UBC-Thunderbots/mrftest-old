#ifndef AI_BACKEND_SIMULATOR_PLAYER_H
#define AI_BACKEND_SIMULATOR_PLAYER_H

#include "ai/backend/backend.h"
#include "simulator/sockproto/proto.h"
#include "util/box_ptr.h"
#include <cstdlib>

namespace AI {
	namespace BE {
		namespace Simulator {
			class Backend;

			/**
			 * \brief A friendly robot that exists in a simulator
			 */
			class Player : public AI::BE::Player, public sigc::trackable {
				public:
					/**
					 * \brief A pointer to a Player
					 */
					typedef BoxPtr<Player> Ptr;

					/**
					 * \brief Constructs a new Player
					 *
					 * \param[in] pattern the pattern index of the robot
					 */
					explicit Player(unsigned int pattern);

					/**
					 * \brief Updates the state of the player and locks in its predictors
					 *
					 * \param[in] state the state block sent by the simulator
					 *
					 * \param[in] ts the timestamp at which the robot was in this position
					 */
					void pre_tick(const ::Simulator::Proto::S2APlayerInfo &state, const AI::Timestamp &ts);

					/**
					 * \brief Encodes the robot's current orders into a packet for transmission to the simulator
					 *
					 * \param[out] orders the packet to encode into
					 */
					void encode_orders(::Simulator::Proto::A2SPlayerInfo &orders);

					void start_drag() { dragging_ = true; }
					void stop_drag() { dragging_ = false; }

					bool highlight() const;
					void dribble_slow();
					bool has_ball() const;
					bool chicker_ready() const;
					void kick_impl(double speed);
					void autokick_impl(double speed);
					void chip_impl(double speed) { kick_impl(speed); }
					void autochip_impl(double speed) { autokick_impl(speed); }
					bool autokick_fired() const;

				private:
					/**
					 * \brief Whether or not this player is holding the ball on its dribbler
					 */
					bool has_ball_;

					/**
					 * \brief Whether the AI elected to kick in the current time tick
					 */
					bool kick_;

					/**
					 * \brief The power level for the kick or chip, if one of the flags is set
					 */
					double chick_power_;

					/**
					 * \brief Whether or not the AI elected to autokick in the last time tick
					 */
					bool autokick_fired_;

					/**
					 * \brief Whether or not the AI elected to autokick in the current time tick
					 */
					bool autokick_pre_fired_;

					bool dragging_;
			};
		}
	}
}

#endif

