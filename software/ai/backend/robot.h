#ifndef AI_BACKEND_ROBOT_H
#define AI_BACKEND_ROBOT_H

#include "ai/flags.h"
#include "ai/common/time.h"
#include "geom/angle.h"
#include "geom/predictor.h"
#include "geom/point.h"
#include "uicomponents/visualizer.h"
#include "util/box_ptr.h"
#include "util/object_store.h"
#include <glibmm/ustring.h>

namespace AI {
	namespace BE {
		/**
		 * \brief A robot, as exposed by the backend
		 */
		class Robot : public Visualizable::Robot {
			public:
				/**
				 * \brief A pointer to a Robot
				 */
				typedef BoxPtr<Robot> Ptr;

				explicit Robot(unsigned int pattern);

				/**
				 * \brief Updates the position of the robot using new field data
				 *
				 * \param[in] pos the position of the robot
				 *
				 * \param[in] ori the orientation of the robot
				 *
				 * \param[in] ts the time at which the robot was in the given position
				 */
				void add_field_data(Point pos, Angle ori, AI::Timestamp ts);

				ObjectStore &object_store() const;
				unsigned int pattern() const;
				Point position() const final override;
				Point position(double delta) const;
				Point position_stdev(double delta) const;
				Angle orientation() const final override;
				Angle orientation(double delta) const;
				Angle orientation_stdev(double delta) const;
				Point velocity() const final override;
				Point velocity(double delta) const;
				Point velocity_stdev(double delta) const;
				Angle avelocity() const;
				Angle avelocity(double delta) const;
				Angle avelocity_stdev(double delta) const;
				bool has_display_path() const override;
				const std::vector<Point> &display_path() const override;
				void avoid_distance(AI::Flags::AvoidDistance dist) const;
				AI::Flags::AvoidDistance avoid_distance() const;
				void pre_tick();
				void lock_time(AI::Timestamp now);

				Visualizable::Colour visualizer_colour() const override;
				Glib::ustring visualizer_label() const final override;
				bool highlight() const override;
				Visualizable::Colour highlight_colour() const override;
				unsigned int num_bar_graphs() const override;
				double bar_graph_value(unsigned int index) const override;
				Visualizable::Colour bar_graph_colour(unsigned int index) const override;

			protected:
				void add_control(Point linear_value, Angle angular_value, Predictor<double>::Timestamp ts);

			private:
				const unsigned int pattern_;
				mutable ObjectStore object_store_;
				mutable AI::Flags::AvoidDistance avoid_distance_;
				Predictor3 pred;
				Point position_cached;
				Angle orientation_cached;
				Point velocity_cached;
				Angle avelocity_cached;

				void update_caches();
		};
	}
}



inline Point AI::BE::Robot::position() const {
	return position_cached;
}

inline Angle AI::BE::Robot::orientation() const {
	return orientation_cached;
}

inline Point AI::BE::Robot::velocity() const {
	return velocity_cached;
}

inline Angle AI::BE::Robot::avelocity() const {
	return avelocity_cached;
}

inline void AI::BE::Robot::avoid_distance(AI::Flags::AvoidDistance dist) const {
	avoid_distance_ = dist;
}

inline AI::Flags::AvoidDistance AI::BE::Robot::avoid_distance() const {
	return avoid_distance_;
}

#endif
