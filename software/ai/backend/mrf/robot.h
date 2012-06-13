#ifndef AI_BACKEND_MRF_ROBOT_H
#define AI_BACKEND_MRF_ROBOT_H

#include "ai/backend/backend.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "uicomponents/visualizer.h"
#include "util/box_ptr.h"
#include "util/predictor.h"
#include <cstdlib>
#include <sigc++/trackable.h>

namespace AI {
	namespace BE {
		namespace MRF {
			/**
			 * \brief A robot, which may or may not be drivable.
			 */
			class Robot : public AI::BE::Robot, public sigc::trackable {
				public:
					/**
					 * \brief A pointer to a Robot.
					 */
					typedef BoxPtr<Robot> Ptr;

					/**
					 * \brief A pointer to a const Robot.
					 */
					typedef BoxPtr<Robot> CPtr;

					/**
					 * \brief Whether or not the robot was seen in one of the most recent camera frames.
					 *
					 * Used internally in the backend.
					 */
					bool seen_this_frame;

					/**
					 * \brief How many times this robot was not visible on any camera.
					 *
					 * Used internally in the backend.
					 */
					unsigned int vision_failures;

					/**
					 * \brief Constructs a new Robot.
					 *
					 * \param[in] backend the backend the robot is part of.
					 *
					 * \param[in] pattern the index of the robot's lid pattern.
					 */
					explicit Robot(AI::BE::Backend &backend, unsigned int pattern);

					/**
					 * \brief Updates the robot with a new SSL-Vision packet.
					 *
					 * \param[in] packet the new packet.
					 *
					 * \param[in] ts the time at which the robot was in the given position.
					 */
					void update(const SSL_DetectionRobot &packet, timespec ts);

					/**
					 * \brief Locks the current time for the predictors.
					 *
					 * \param[in] now the current time.
					 */
					void lock_time(timespec now);

					Visualizable::Colour visualizer_colour() const;
					Glib::ustring visualizer_label() const;
					bool highlight() const;
					Visualizable::Colour highlight_colour() const;
					Point position(double delta = 0.0) const;
					Point velocity(double delta = 0.0) const;
					Point position_stdev(double delta = 0.0) const;
					Point velocity_stdev(double delta = 0.0) const;
					Angle orientation(double delta = 0.0) const;
					Angle avelocity(double delta = 0.0) const;
					Angle orientation_stdev(double delta = 0.0) const;
					Angle avelocity_stdev(double delta = 0.0) const;
					unsigned int pattern() const;
					ObjectStore &object_store() const;
					bool has_destination() const;
					const std::pair<Point, Angle> &destination() const;
					bool has_path() const;
					const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &path() const;
					unsigned int num_bar_graphs() const;
					double bar_graph_value(unsigned int index) const;
					Visualizable::Colour bar_graph_colour(unsigned int index) const;

				protected:
					AI::BE::Backend &backend;

				private:
					const unsigned int pattern_;
					Predictor3 pred;
					mutable ObjectStore object_store_;

					void on_defending_end_changed();
			};
		}
	}
}

#endif

