#ifndef AI_BACKEND_SIMULATOR_BALL_H
#define AI_BACKEND_SIMULATOR_BALL_H

#include "ai/backend/backend.h"
#include "simulator/sockproto/proto.h"
#include "util/predictor.h"

namespace AI {
	namespace BE {
		namespace Simulator {
			/**
			 * A ball whose position is provided by a simulator.
			 */
			class Ball : public AI::BE::Ball {
				public:
					/**
					 * Constructs a new Ball.
					 */
					Ball() : xpred(false), ypred(false) {
					}

					/**
					 * Destroys a Ball.
					 */
					~Ball() {
					}

					/**
					 * Pushes a new position datum into the ball and locks the predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] ts the timestamp at which the ball was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2ABallInfo &state, const timespec &ts) {
						xpred.add_datum(state.x, ts);
						xpred.lock_time(ts);
						ypred.add_datum(state.y, ts);
						ypred.lock_time(ts);
					}

					Point position() const { return Point(xpred.value(), ypred.value()); }
					Point position(double delta) const { return Point(xpred.value(delta), ypred.value(delta)); }
					Point position(const timespec &ts) const { return Point(xpred.value(ts), ypred.value(ts)); }
					Point velocity() const { return Point(xpred.value(0.0, 1), ypred.value(0.0, 1)); }
					Point velocity(double delta) const { return Point(xpred.value(delta, 1), ypred.value(delta, 1)); }
					Point velocity(const timespec &ts) const { return Point(xpred.value(ts, 1), ypred.value(ts, 1)); }
					Point acceleration(double delta) const { return Point(xpred.value(delta, 2), ypred.value(delta, 2)); }
					Point acceleration(const timespec &ts) const { return Point(xpred.value(ts, 2), ypred.value(ts, 2)); }

				private:
					/**
					 * A predictor that provides the X coordinate of predictable quantities.
					 */
					Predictor xpred;

					/**
					 * A predictor that provides the Y coordinate of predictable quantities.
					 */
					Predictor ypred;
			};
		}
	}
}

#endif

