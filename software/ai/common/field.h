#ifndef AI_COMMON_FIELD_H
#define AI_COMMON_FIELD_H

#include"geom/point.h"
#include <utility>

namespace AI {
	namespace Common {
		/**
		 * Exposes the dimensions of various parts of the field.
		 */
		class Field {
			public:
				/**
				 * Checks if the field data is valid yet.
				 *
				 * \return \c true if the data in the Field is valid, or \c false if not.
				 */
				virtual bool valid() const = 0;

				/**
				 * Gets the length of the field from goal to goal.
				 *
				 * \return the length of the field in metres.
				 */
				virtual double length() const = 0;

				/**
				 * Gets the length of the field including the boundary and referee area.
				 *
				 * \return the length of the field in metres.
				 */
				virtual double total_length() const = 0;

				/**
				 * Gets the width of the field from sideline to sideline.
				 *
				 * \return the width of the field in metres.
				 */
				virtual double width() const = 0;

				/**
				 * Gets the width of the field including the boundary and referee area.
				 *
				 * \return the width of the field in metres.
				 */
				virtual double total_width() const = 0;

				/**
				 * Gets the width of the goal, symmetric above and below the centreline, from goalpost to goalpost.
				 *
				 * \return the width of the goal in metres.
				 */
				virtual double goal_width() const = 0;

				/**
				 * Gets the radius of the centre circle.
				 *
				 * \return the radius of the centre circle in metres.
				 */
				virtual double centre_circle_radius() const = 0;

				/**
				 * Gets the radius of the arcs at the top and bottom of the defense areas.
				 *
				 * \return the radius of the arcs in metres.
				 */
				virtual double defense_area_radius() const = 0;

				/**
				 * Gets the width of the straight parts of the defense areas between their pairs of arcs.
				 *
				 * \return the width of the straight parts in metres.
				 */
				virtual double defense_area_stretch() const = 0;

				/**
				 * Gets the position of the centre of the friendly goal.
				 *
				 * \return the position of the friendly goal.
				 */
				Point friendly_goal() const;

				/**
				 * Gets the position of the centre of the enemy goal.
				 *
				 * \return the position of the enemy goal.
				 */
				Point enemy_goal() const;

				/**
				 * Gets the position of the penalty mark near the enemy goal.
				 *
				 * \return the position of the penalty mark, 450mm from the centre of the enemy goal.
				 */
				Point penalty_enemy() const;

				/**
				 * Gets the position of the penalty mark near the friendly goal.
				 *
				 * \return the position of the penalty mark, 450 mm from the centre of the friendly goal.
				 */
				Point penalty_friendly() const;

				/**
				 * Gets the positions of the friendly goalposts.
				 *
				 * \return the goalpost positions, top and bottom.
				 */
				std::pair<Point, Point> friendly_goal_boundary() const;

				/**
				 * Gets the positions of the enemy goalposts.
				 *
				 * \return the goalpost positions, top and bottom.
				 */
				std::pair<Point, Point> enemy_goal_boundary() const;

				/**
				 * Gets the margin for being out of bounds on the top or bottom of the field.
				 *
				 * \return the margin, in metres.
				 */
				double bounds_margin() const;
		};
	}
}

#endif

