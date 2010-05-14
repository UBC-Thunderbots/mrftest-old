#ifndef AI_NAVIGATOR_BASIC_NAVIGATOR_H
#define AI_NAVIGATOR_BASIC_NAVIGATOR_H

#include "ai/navigator.h"

/**
 * Basic collision avoidance navigator.
 * Should not have complicated functionality.
 */
class basic_navigator : public navigator {
	public:
		basic_navigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team);
		void tick();
		void set_point(const point& destination);

		/**
		 * Helper function to clip a point on boundaries.
		 */
		static point clip_point(const point& p, const point& bound1, const point& bound2);

	protected:

		bool check_vector(const point& start, const point& dest, const point& direction) const;

		///has a destination been specified?
		bool dest_initialized;

		///current destination
		point curr_dest;

		//distance to remain from sidelines to prevent from going oob
		double outofbounds_margin;
		double max_lookahead;

		/// Smaller factor makes robot more aggressive (i.e. less eager to avoid obstacles).
		/// This multiplied by robot diameter determines how far it must avoid
		double aggression_factor;

		/// Checks if destination is reached. If so, ignore collisions.
		bool reached;

		/// how much it should rotate per vector check
		double rotation_angle;

		/// how much it should rotate until it gives up
		double rotation_thresh;
};

#endif

