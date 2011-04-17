#ifndef AI_WORLD_FIELD_H
#define AI_WORLD_FIELD_H

#include "ai/backend/backend.h"
#include "proto/messages_robocup_ssl_geometry.pb.h"
#include "uicomponents/visualizer.h"
#include "util/noncopyable.h"

namespace AI {
	namespace BE {
		namespace XBee {
			/**
			 * The XBee implementation of the AI::BE::Field class.
			 */
			class Field : public AI::BE::Field {
				public:
					/**
					 * Constructs a new Field.
					 */
					Field();

					/**
					 * Updates the Field object with new geometry data from SSL-Vision or the simulator.
					 *
					 * \param packet packet the new data.
					 */
					void update(const SSL_GeometryFieldSize &packet);

					/**
					 * Checks if the field data is valid yet.
					 *
					 * \return \c true if the data in the Field is valid, or \c false if not.
					 */
					bool valid() const {
						return valid_;
					}

					/**
					 * Gets the length of the field from goal to goal.
					 *
					 * \return the length of the field in metres.
					 */
					double length() const {
						return length_;
					}

					/**
					 * Gets the length of the field including the boundary and referee area.
					 *
					 * \return the length of the field in metres.
					 */
					double total_length() const {
						return total_length_;
					}

					/**
					 * Gets the width of the field from sideline to sideline.
					 *
					 * \return the width of the field in metres.
					 */
					double width() const {
						return width_;
					}

					/**
					 * Gets the width of the field including the boundary and referee area.
					 *
					 * \return the width of the field in metres.
					 */
					double total_width() const {
						return total_width_;
					}

					/**
					 * Gets the width of the goal, symmetric above and below the centreline, from goalpost to goalpost.
					 *
					 * \return the width of the goal in metres.
					 */
					double goal_width() const {
						return goal_width_;
					}

					/**
					 * Gets the radius of the centre circle.
					 *
					 * \return the radius of the centre circle in metres.
					 */
					double centre_circle_radius() const {
						return centre_circle_radius_;
					}

					/**
					 * Gets the radius of the arcs at the top and bottom of the defense areas.
					 *
					 * \return the radius of the arcs in metres.
					 */
					double defense_area_radius() const {
						return defense_area_radius_;
					}

					/**
					 * Gets the width of the straight parts of the defense areas between their pairs of arcs.
					 *
					 * \return the width of the straight parts in metres.
					 */
					double defense_area_stretch() const {
						return defense_area_stretch_;
					}

					/**
					 * Gets the position of the centre of the friendly goal.
					 *
					 * \return the position of the friendly goal.
					 */
					Point friendly_goal() const {
						return Point(-length_ * 0.5, 0);
					}

					/**
					 * Gets the position of the centre of the enemy goal.
					 *
					 * \return the position of the enemy goal.
					 */
					Point enemy_goal() const {
						return Point(length_ * 0.5, 0);
					}

					/**
					 * Gets the position of the penalty mark near the enemy goal.
					 *
					 * \return the position of the penalty mark, 450mm from the centre of the enemy goal.
					 */
					Point penalty_enemy() const {
						return Point(length_ * 0.5 / 3.025 * (3.025 - 0.450), 0);
					}

					/**
					 * Gets the position of the penalty mark near the friendly goal.
					 *
					 * \return the position of the penalty mark, 450 mm from the centre of the friendly goal.
					 */
					Point penalty_friendly() const {
						return Point(-length_ * 0.5 / 3.025 * (3.025 - 0.450), 0);
					}

					/**
					 * Gets the positions of the friendly goalposts.
					 *
					 * \return the goalpost positions, top and bottom.
					 */
					std::pair<Point, Point> friendly_goal_boundary() const {
						return std::make_pair(Point(-length_ * 0.5, -0.5 * goal_width_), Point(-length_ * 0.5, 0.5 * goal_width_));
					}

					/**
					 * Gets the positions of the enemy goalposts.
					 *
					 * \return the goalpost positions, top and bottom.
					 */
					std::pair<Point, Point> enemy_goal_boundary() const {
						return std::make_pair(Point(length_ * 0.5, -0.5 * goal_width_), Point(length_ * 0.5, 0.5 * goal_width_));
					}

					/**
					 * Gets the margin for being out of bounds on the top or bottom of the field.
					 *
					 * \return the margin, in metres.
					 */
					double bounds_margin() const {
						return width_ / 20.0;
					}

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
			};
		}
	}
}

#endif

