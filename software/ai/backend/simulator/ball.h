#ifndef AI_BACKEND_SIMULATOR_BALL_H
#define AI_BACKEND_SIMULATOR_BALL_H

#include "ai/backend/backend.h"
#include "simulator/sockproto/proto.h"
#include "util/predictor.h"

namespace AI {
	namespace BE {
		namespace Simulator {
			class Backend;

			/**
			 * A ball whose position is provided by a simulator.
			 */
			class Ball : public AI::BE::Ball, public sigc::trackable {
				public:
					/**
					 * Constructs a new Ball.
					 *
					 * \param[in] be the backend containing the ball.
					 */
					Ball(Backend &be);

					/**
					 * Pushes a new position datum into the ball and locks the predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] ts the timestamp at which the ball was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2ABallInfo &state, const timespec &ts);

					/**
					 * Indicates that the mouse was pressed over the visualizer.
					 *
					 * \param[in] p the point, in world coordinates, over which the mouse was pressed.
					 *
					 * \param[in] btn the number of the button that was pressed.
					 */
					void mouse_pressed(Point p, unsigned int btn);

					/**
					 * Indicates that the mouse was released over the visualizer.
					 *
					 * \param[in] p the point, in world coordinates, over which the mouse was released.
					 *
					 * \param[in] btn the number of the button that was released.
					 */
					void mouse_released(Point p, unsigned int btn);

					/**
					 * Indicates that the mouse exited the area of the visualizer.
					 */
					void mouse_exited();

					/**
					 * Indicates that the mouse was moved over the visualizer.
					 *
					 * \param[in] p the new position of the mouse pointer, in world coordinates.
					 */
					void mouse_moved(Point p);

					Point position(double delta = 0.0) const;
					Point velocity(double delta = 0.0) const;
					Point acceleration(double delta = 0.0) const;
					bool highlight() const;
					Visualizable::Colour highlight_colour() const;

				private:
					/**
					 * The backend.
					 */
					Backend &be;

					/**
					 * A predictor that provides the X coordinate of predictable quantities.
					 */
					Predictor xpred;

					/**
					 * A predictor that provides the Y coordinate of predictable quantities.
					 */
					Predictor ypred;

					/**
					 * The connections for mouse activity signals.
					 */
					sigc::connection mouse_connections[3];

					/**
					 * Disconnects all mouse activity signals except press.
					 */
					void disconnect_mouse();
			};
		}
	}
}

#endif

