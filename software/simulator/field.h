#ifndef SIM_FIELD_H
#define SIM_FIELD_H

/**
 * The dimensions of the field.
 */
class SimulatorField {
	public:
		/**
		 * The length of the field, from goal to goal, in metres.
		 */
		static const double length;

		/**
		 * The length of the field, including the boundary and referee area.
		 */
		static const double total_length;

		/**
		 * The width of the field, from sideline to sideline, in the Y direction.
		 */
		static const double width;

		/**
		 * The width of the field, including the boundary and referee area.
		 */
		static const double total_width;

		/**
		 * The width of the goal, symmetric above and below the centreline.
		 */
		static const double goal_width;

		/**
		 * The radius of the centre circle.
		 */
		static const double centre_circle_radius;

		/**
		 * The radius of the arcs at the top and bottom of the defense area.
		 */
		static const double defense_area_radius;

		/**
		 * The width of the straight part between the arcs in the defense area.
		 */
		static const double defense_area_stretch;
};

#endif

