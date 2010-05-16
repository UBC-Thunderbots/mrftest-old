#ifndef AI_WORLD_ROBOT_H
#define AI_WORLD_ROBOT_H

#include "ai/world/predictable.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "util/byref.h"
#include <glibmm.h>

class ai;
class world;

/**
 * A robot, which may or may not be drivable.
 */
class robot : public byref, public predictable {
	public:
		/**
		 * A pointer to a robot.
		 */
		typedef Glib::RefPtr<robot> ptr;

		/**
		 * The largest possible radius of a robot, in metres.
		 */
		static const double MAX_RADIUS = 0.09;

		/**
		 * The colour of the robot.
		 */
		const bool yellow;

		/**
		 * The index of the SSL-Vision lid pattern.
		 */
		const unsigned int pattern_index;

	protected:
		double sign;

		/**
		 * Constructs a new player object.
		 * \param bot the XBee robot being driven
		 */
		robot(bool yellow, unsigned int pattern_index);

	private:
		unsigned int vision_failures;
		bool seen_this_frame;

		/**
		 * Constructs a new non-drivable robot object.
		 * \return the new object
		 */
		static ptr create(bool yellow, unsigned int pattern_index);

		/**
		 * Updates the position of the player using new data.
		 * \param packet the new data to update with
		 */
		void update(const SSL_DetectionRobot &packet);

		friend class ai;
		friend class world;
};

#endif

