#ifndef SIM_BALL_H
#define SIM_BALL_H

#include "geom/point.h"
#include "uicomponents/visualizer.h"
#include "util/byref.h"
#include "util/fd.h"
#include <glibmm.h>

namespace Simulator {
	/**
	 * The ball, as seen by a simulation engine.
	 * An individual engine is expected to subclass this class and return an instance of the subclass from its SimulatorEngine::get_ball() method.
	 */
	class Ball : public ByRef {
		public:
			/**
			 * A pointer to a Ball.
			 */
			typedef RefPtr<Ball> Ptr;

			/**
			 * Returns the position of the ball.
			 *
			 * \return the position of the ball, in metres from field centre.
			 */
			virtual Point position() const __attribute__((warn_unused_result)) = 0;

			/**
			 * Moves the ball.
			 *
			 * \param[in] pos the new location of the ball, in metres from field centre.
			 */
			virtual void position(const Point &pos) = 0;

			/**
			 * Returns the velocity of the ball.
			 *
			 * \return the velocity of the ball, in metres per second.
			 */
			virtual Point velocity() const __attribute__((warn_unused_result)) = 0;

			/**
			 * Sets the ball's velocity.
			 *
			 * \param[in] vel the new velocity, in metres per second field-relative.
			 */
			virtual void velocity(const Point &vel) = 0;

			/**
			 * Loads the ball's state from a file.
			 *
			 * \param[in] fd the file to load from.
			 */
			virtual void load_state(FileDescriptor::Ptr fd) = 0;

			/**
			 * Saves the ball's state to a file.
			 *
			 * \param[in] fd the file to save to.
			 */
			virtual void save_state(FileDescriptor::Ptr fd) const = 0;

		protected:
			/**
			 * Constructs a new Ball.
			 */
			Ball() {
			}

			/**
			 * Destroys a Ball.
			 */
			~Ball() {
			}

		private:
			bool visualizer_can_drag() const {
				return true;
			}

			void visualizer_drag(const Point &pos) {
				position(pos);
			}
	};
}

#endif

