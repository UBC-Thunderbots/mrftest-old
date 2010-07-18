#ifndef AI_WORLD_FIELD_H
#define AI_WORLD_FIELD_H

#include "proto/messages_robocup_ssl_geometry.pb.h"
#include "uicomponents/visualizer.h"
#include "util/noncopyable.h"

class World;

/**
 * Provides the AI with a way to get the dimensions of the field. The dimensions
 * of the field are provided by SSL-Vision or the simulator.
 */
class Field : public Visualizable::Field {
	public:
		/**
		 * \return true if the data in the Field is valid, or false if not
		 */
		bool valid() const {
			return valid_;
		}

		/**
		 * \return The length of the field, from goal to goal, in metres
		 */
		double length() const {
			return length_;
		}

		/**
		 * \return The length of the field, including the boundary and referee
		 * area, in metres
		 */
		double total_length() const {
			return total_length_;
		}

		/**
		 * \return The width of the field, from sideline to sideline, in metres
		 */
		double width() const {
			return width_;
		}

		/**
		 * \return The width of the field, including the boundary and referee
		 * area, in metres
		 */
		double total_width() const {
			return total_width_;
		}

		/**
		 * \return The width of the goal, symmetric above and below the
		 * centreline, from goalpost to goalpost, in metres
		 */
		double goal_width() const {
			return goal_width_;
		}

		/**
		 * \return The radius of the centre circle, in metres
		 */
		double centre_circle_radius() const {
			return centre_circle_radius_;
		}

		/**
		 * \return The radius of the arcs at the top and bottom of the defense
		 * area, in metres
		 */
		double defense_area_radius() const {
			return defense_area_radius_;
		}

		/**
		 * \return The width of the straight part of the defense area between
		 * the two arcs, in metres
		 */
		double defense_area_stretch() const {
			return defense_area_stretch_;
		}

		/**
		 * \return Position of our own goal.
		 */
		Point friendly_goal() const {
			return Point(-length_ * 0.5, 0);
		}

		/**
		 * \return Position of enemy goal.
		 */
		Point enemy_goal() const {
			return Point(length_ * 0.5, 0);
		}

		/**
		 * \return Position of enemy penalty mark.
		 * 450 mm from enemy goal center.
		 */
		Point penalty_enemy() const {
			return Point(length_ * 0.5 / 3025.0 * (3025.0 - 450.0), 0);
		}

		/**
		 * \return Position of friendly penalty mark.
		 * 450 mm from friendly goal center.
		 */
		Point penalty_friendly() const {
			return Point(-length_ * 0.5 / 3025.0 * (3025.0 - 450.0), 0);
		}

		/**
		 * \return Position of our goal boundaries (top and bottom).
		 */
		std::pair<Point, Point> friendly_goal_boundary() const {
			return std::make_pair(Point(-length_ * 0.5, -0.5*goal_width_), Point(-length_ * 0.5, 0.5*goal_width_));
		}

		/**
		 * \return Position of enemy goal boundaries (top and bottom).
		 */
		std::pair<Point, Point> enemy_goal_boundary() const {
			return std::make_pair(Point(length_ * 0.5, -0.5*goal_width_), Point(length_ * 0.5, 0.5*goal_width_));
		}

		/**
		 * \return Out of bounds margin.
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

		/**
		 * Constructs a new Field object.
		 */
		Field();

		/**
		 * Updates the Field object with new geometry data from SSL-Vision or
		 * the simulator.
		 *
		 * \param packet packet the new data
		 */
		void update(const SSL_GeometryFieldSize &packet);

		friend class World;
};

#endif

