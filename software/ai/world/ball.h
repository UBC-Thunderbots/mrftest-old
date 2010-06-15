#ifndef AI_WORLD_BALL_H
#define AI_WORLD_BALL_H

#include "ai/world/predictable.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "uicomponents/visualizer.h"
#include "util/byref.h"
#include <cstdlib>
#include <glibmm.h>

class world;

/**
 * The ball.
 */
class ball : public visualizable::ball, public predictable {
	public:
		/**
		 * A pointer to a ball.
		 */
		typedef Glib::RefPtr<ball> ptr;

		/**
		 * The approximate radius of the ball.
		 */
		static const double RADIUS;

		/**
		 * \return the position of the robot.
		 */
		point position() const {
			return predictable::position();
		}

	private:
		double sign;

		/**
		 * Constructs a new ball object.
		 *
		 * \return the new object.
		 */
		static ptr create();

		/**
		 * Constructs a new ball object.
		 */
		ball();

		/**
		 * Updates the position of the ball using new data.
		 *
		 * \param[in] pos the new position of the ball, in unswapped field
		 * coordinates.
		 */
		void update(const point &pos);

		bool visualizer_can_drag() const {
			return false;
		}

		void visualizer_drag(const point &) {
			std::abort();
		}

		point velocity() const {
			return est_velocity();
		}

		friend class world;
};

#endif

