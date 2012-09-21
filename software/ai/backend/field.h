#ifndef AI_BACKEND_FIELD_H
#define AI_BACKEND_FIELD_H

#include "ai/common/field.h"
#include "uicomponents/visualizer.h"

namespace AI {
	namespace BE {
		/**
		 * \brief The field, as exposed by the backend
		 */
		class Field : public AI::Common::Field, public Visualizable::Field {
			public:
				/**
				 * \brief Constructs a new Field
				 */
				explicit Field();

				/**
				 * \brief Updates the Field object with new geometry data from SSL-Vision or the simulator
				 *
				 * \param[in] length the length of the field
				 *
				 * \param[in] total_length the length of the field including boundary and referee area
				 *
				 * \param[in] width the width of the field
				 *
				 * \param[in] total_width the width of the field including boundary and referee area
				 *
				 * \param[in] centre_circle_radius the radius of the centre circle
				 *
				 * \param[in] defense_area_radius the radius of the defense area
				 *
				 * \param[in] defense_area_stretch the width of the straight part of the defense area
				 */
				void update(double length, double total_length, double width, double total_width, double goal_width, double centre_circle_radius, double defense_area_radius, double defense_area_stretch);

				/**
				 * \brief Checks if the field data is valid yet
				 *
				 * \return \c true if the data in the Field is valid, or \c false if not
				 */
				bool valid() const;

				/**
				 * \brief Gets the length of the field from goal to goal
				 *
				 * \return the length of the field in metres
				 */
				double length() const;

				/**
				 * \brief Gets the length of the field including the boundary and referee area
				 *
				 * \return the length of the field in metres
				 */
				double total_length() const;

				/**
				 * \brief Gets the width of the field from sideline to sideline
				 *
				 * \return the width of the field in metres
				 */
				double width() const;

				/**
				 * \brief Gets the width of the field including the boundary and referee area
				 *
				 * \return the width of the field in metres
				 */
				double total_width() const;

				/**
				 * \brief Gets the width of the goal, symmetric above and below the centreline, from goalpost to goalpost
				 *
				 * \return the width of the goal in metres
				 */
				double goal_width() const;

				/**
				 * \brief Gets the radius of the centre circle
				 *
				 * \return the radius of the centre circle in metres
				 */
				double centre_circle_radius() const;

				/**
				 * \brief Gets the radius of the arcs at the top and bottom of the defense areas
				 *
				 * \return the radius of the arcs in metres
				 */
				double defense_area_radius() const;

				/**
				 * \brief Gets the width of the straight parts of the defense areas between their pairs of arcs
				 *
				 * \return the width of the straight parts in metres
				 */
				double defense_area_stretch() const;

				/**
				 * \brief Gets the position of the centre of the friendly goal
				 *
				 * \return the position of the friendly goal
				 */
				Point friendly_goal() const;

				/**
				 * \brief Gets the position of the centre of the enemy goal
				 *
				 * \return the position of the enemy goal
				 */
				Point enemy_goal() const;

				/**
				 * \brief Gets the position of the penalty mark near the enemy goal
				 *
				 * \return the position of the penalty mark near the enemy goal
				 */
				Point penalty_enemy() const;

				/**
				 * \brief Gets the position of the penalty mark near the friendly goal
				 *
				 * \return the position of the penalty mark near the friendly goal
				 */
				Point penalty_friendly() const;

				/**
				 * \brief Gets the positions of the friendly goalposts
				 *
				 * \return the goalpost positions, top and bottom
				 */
				std::pair<Point, Point> friendly_goal_boundary() const;

				/**
				 * \brief Gets the positions of the enemy goalposts
				 *
				 * \return the goalpost positions, top and bottom
				 */
				std::pair<Point, Point> enemy_goal_boundary() const;

				/**
				 * \brief Gets the margin for being out of bounds on the top or bottom of the field
				 *
				 * \return the margin, in metres
				 */
				double bounds_margin() const;

			private:
				bool valid_;
				double length_;
				double total_length_;
				double width_;
				double total_width_;
				double goal_width_;
				double centre_circle_radius_;
				double defense_area_radius_;
				double defense_area_stretch_;

				bool operator==(const Field &other) const;
				bool operator!=(const Field &other) const;
		};
	}
}



inline bool AI::BE::Field::valid() const {
	return valid_;
}

inline double AI::BE::Field::length() const {
	return length_;
}

inline double AI::BE::Field::total_length() const {
	return total_length_;
}

inline double AI::BE::Field::width() const {
	return width_;
}

inline double AI::BE::Field::total_width() const {
	return total_width_;
}

inline double AI::BE::Field::goal_width() const {
	return goal_width_;
}

inline double AI::BE::Field::centre_circle_radius() const {
	return centre_circle_radius_;
}

inline double AI::BE::Field::defense_area_radius() const {
	return defense_area_radius_;
}

inline double AI::BE::Field::defense_area_stretch() const {
	return defense_area_stretch_;
}

inline Point AI::BE::Field::friendly_goal() const {
	return Point(-length_ * 0.5, 0);
}

inline Point AI::BE::Field::enemy_goal() const {
	return Point(length_ * 0.5, 0);
}

inline Point AI::BE::Field::penalty_enemy() const {
	return Point(length_ * 0.5 / 3.025 * (3.025 - 0.750), 0);
}

inline Point AI::BE::Field::penalty_friendly() const {
	return Point(-length_ * 0.5 / 3.025 * (3.025 - 0.750), 0);
}

inline std::pair<Point, Point> AI::BE::Field::friendly_goal_boundary() const {
	return std::make_pair(Point(-length_ * 0.5, -0.5 * goal_width_), Point(-length_ * 0.5, 0.5 * goal_width_));
}

inline std::pair<Point, Point> AI::BE::Field::enemy_goal_boundary() const {
	return std::make_pair(Point(length_ * 0.5, -0.5 * goal_width_), Point(length_ * 0.5, 0.5 * goal_width_));
}

inline double AI::BE::Field::bounds_margin() const {
	return width_ / 20.0;
}

#endif

