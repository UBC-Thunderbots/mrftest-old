#ifndef SIM_BALL_H
#define SIM_BALL_H

#include "geom/point.h"
#include "uicomponents/visualizer.h"
#include "util/byref.h"
#include <glibmm.h>

/**
 * The ball, as seen by a simulation engine. An individual engine is expected to
 * subclass this class and return an instance of the subclass from its
 * SimulatorEngine::get_ball() method.
 */
class SimulatorBall : public Visualizable::Ball {
	public:
		/**
		 * A pointer to a SimulatorBall.
		 */
		typedef Glib::RefPtr<SimulatorBall> ptr;

		/**
		 * \return The position of the ball, in metres from field centre
		 */
		virtual Point position() const __attribute__((warn_unused_result)) = 0;

		/**
		 * Moves the ball.
		 * \param pos the new location of the ball, in metres from field centre
		 */
		virtual void position(const Point &pos) = 0;

		/**
		 * Sets the ball's velocity.
		 * \param vel the new velocity, in metres per second field-relative
		 */
		virtual void velocity(const Point &vel) = 0;

	private:
		bool visualizer_can_drag() const {
			return true;
		}

		void visualizer_drag(const Point &pos) {
			position(pos);
		}
};

#endif

