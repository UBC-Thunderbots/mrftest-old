#ifndef AI_WORLD_BALL_H
#define AI_WORLD_BALL_H

#include "ai/world/predictable.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "uicomponents/visualizer.h"
#include "util/byref.h"
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
		static const double RADIUS = 0.0215;

		/**
		 * \return The position of the robot
		 */
		point position() const {
			return predictable::position();
		}

	private:
		double sign;

		/**
		 * Constructs a new ball object.
		 * \return The new object
		 */
		static ptr create();

		/**
		 * Constructs a new ball object.
		 */
		ball();

		/**
		 * Updates the position of the ball using new data.
		 * \param packet the new data to update with
		 */
		void update(const SSL_DetectionBall &packet);

		friend class world;
};

#endif

