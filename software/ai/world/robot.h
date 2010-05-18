#ifndef AI_WORLD_ROBOT_H
#define AI_WORLD_ROBOT_H

#include "ai/world/predictable.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "uicomponents/visualizer.h"
#include "util/byref.h"
#include <glibmm.h>

class ai;
class world;

/**
 * A robot, which may or may not be drivable.
 */
class robot : public visualizable::robot, public predictable {
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

		/**
		 * \return The position of the robot.
		 */
		point position() const {
			return predictable::position();
		}

		/**
		 * \return The orientation of the robot.
		 */
		double orientation() const {
			return predictable::orientation();
		}

		visualizable::colour visualizer_colour() const {
			// Enemies are red; overridden in subclass for friendlies.
			return visualizable::colour(1.0, 0.0, 0.0);
		}

		Glib::ustring visualizer_label() const {
			return Glib::ustring::compose("%1%2", yellow ? 'Y' : 'B', pattern_index);
		}

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

