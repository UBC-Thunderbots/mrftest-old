#ifndef AI_WORLD_ROBOT_H
#define AI_WORLD_ROBOT_H

#include "ai/world/predictable.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "uicomponents/visualizer.h"
#include "util/byref.h"
#include <cstdlib>
#include <glibmm.h>

class AI;
class World;

/**
 * A robot, which may or may not be drivable.
 */
class Robot : public Visualizable::Robot, public Predictable, public sigc::trackable {
	public:
		/**
		 * A pointer to a Robot.
		 */
		typedef RefPtr<Robot> Ptr;

		/**
		 * The largest possible radius of a robot, in metres.
		 */
		static const double MAX_RADIUS;

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
		Point position() const {
			return Predictable::position();
		}

		/**
		 * \return The orientation of the robot.
		 */
		double orientation() const {
			return Predictable::orientation();
		}

	protected:
		double sign;

		/**
		 * Constructs a new Robot object.
		 *
		 * \param[in] yellow the colour of the new robot.
		 *
		 * \param[in] pattern_index the lid pattern index of the new robot.
		 */
		Robot(bool yellow, unsigned int pattern_index);

	private:
		unsigned int vision_failures;
		bool seen_this_frame;

		/**
		 * Constructs a new non-drivable Robot object.
		 * \return the new object
		 */
		static Ptr create(bool yellow, unsigned int pattern_index);

		/**
		 * Updates the position of the Robot using new data.
		 * \param packet the new data to update with
		 */
		void update(const SSL_DetectionRobot &packet);

		bool visualizer_visible() const {
			return true;
		}

		Visualizable::RobotColour visualizer_colour() const {
			// Enemies are red; overridden in subclass for friendlies.
			return Visualizable::RobotColour(1.0, 0.0, 0.0);
		}

		Glib::ustring visualizer_label() const {
			return Glib::ustring::compose("%1%2", yellow ? 'Y' : 'B', pattern_index);
		}

		bool has_destination() const {
			return false;
		}

		Point destination() const {
			std::abort();
		}

		bool visualizer_can_drag() const {
			return false;
		}

		void visualizer_drag(const Point &) {
			std::abort();
		}

		friend class AI;
		friend class World;
};

#endif

