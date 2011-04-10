#ifndef AI_BACKEND_XBEE_ROBOT_H
#define AI_BACKEND_XBEE_ROBOT_H

#include "ai/backend/backend.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "uicomponents/visualizer.h"
#include "util/predictor.h"
#include <cstdlib>
#include <sigc++/sigc++.h>

namespace AI {
	namespace BE {
		namespace XBee {
			/**
			 * A robot, which may or may not be drivable.
			 */
			class Robot : public AI::BE::Robot, public sigc::trackable {
				public:
					/**
					 * A pointer to a Robot.
					 */
					typedef RefPtr<Robot> Ptr;

					/**
					 * A pointer to a const Robot.
					 */
					typedef RefPtr<const Robot> CPtr;

					/**
					 * Whether or not the robot was seen in one of the most recent camera frames.
					 * Used internally in the backend.
					 */
					bool seen_this_frame;

					/**
					 * How many times this robot was not visible on any camera.
					 * Used internally in the backend.
					 */
					unsigned int vision_failures;

					/**
					 * Constructs a new Robot.
					 *
					 * \param[in] backend the backend the robot is part of.
					 *
					 * \param[in] pattern the index of the robot's lid pattern.
					 *
					 * \return the new Robot.
					 */
					static Ptr create(AI::BE::Backend &backend, unsigned int pattern);

					/**
					 * Updates the robot with a new SSL-Vision packet.
					 *
					 * \param[in] packet the new packet.
					 *
					 * \param[in] ts the time at which the robot was in the given position.
					 */
					void update(const SSL_DetectionRobot &packet, const timespec &ts);

					/**
					 * Locks the current time for the predictors.
					 *
					 * \param[in] now the current time.
					 */
					void lock_time(const timespec &now);

					Visualizable::Colour visualizer_colour() const;
					Glib::ustring visualizer_label() const;
					bool highlight() const;
					Visualizable::Colour highlight_colour() const;
					Point position() const { return position(0.0); }
					Point position(double delta) const;
					Point position(const timespec &ts) const;
					Point velocity() const { return velocity(0.0); }
					Point velocity(double delta = 0.0) const;
					Point velocity(const timespec &ts) const;
					Point acceleration(double delta = 0.0) const;
					Point acceleration(const timespec &ts) const;
					double orientation() const { return orientation(0.0); }
					double orientation(double delta) const;
					double orientation(const timespec &ts) const;
					double avelocity(double delta = 0.0) const;
					double avelocity(const timespec &ts) const;
					double aacceleration(double delta = 0.0) const;
					double aacceleration(const timespec &ts) const;
					unsigned int pattern() const;
					ObjectStore &object_store() const;
					bool has_destination() const;
					const std::pair<Point, double> &destination() const;
					bool has_path() const;
					const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const;

				protected:
					AI::BE::Backend &backend;

					Robot(AI::BE::Backend &backend, unsigned int pattern);
					~Robot();

				private:
					const unsigned int pattern_;
					Predictor xpred, ypred, tpred;
					mutable ObjectStore object_store_;

					void on_defending_end_changed();
			};
		}
	}
}

#endif

