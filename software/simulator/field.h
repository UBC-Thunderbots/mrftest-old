#ifndef SIM_FIELD_H
#define SIM_FIELD_H

/**
 * The dimensions of the field.
 */
namespace SimulatorField {
	/**
	 * The length of the field, from goal to goal, in metres.
	 */
	extern const double LENGTH;

	/**
	 * The length of the field, including the boundary and referee area.
	 */
	extern const double TOTAL_LENGTH;

	/**
	 * The width of the field, from sideline to sideline, in the Y direction.
	 */
	extern const double WIDTH;

	/**
	 * The width of the field, including the boundary and referee area.
	 */
	extern const double TOTAL_WIDTH;

	/**
	 * The width of the goal, symmetric above and below the centreline.
	 */
	extern const double GOAL_WIDTH;

	/**
	 * The radius of the centre circle.
	 */
	extern const double CENTRE_CIRCLE_RADIUS;

	/**
	 * The radius of the arcs at the top and bottom of the defense area.
	 */
	extern const double DEFENSE_AREA_RADIUS;

	/**
	 * The width of the straight part between the arcs in the defense area.
	 */
	extern const double DEFENSE_AREA_STRETCH;
};

#endif

