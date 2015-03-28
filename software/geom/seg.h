#pragma once

#include "geom/point.h"
#include "geom/line.h"

namespace Geom {
	class Seg final {
	public:
		Vector2 start;
		Vector2 end;

		/**
		 * \brief Creates a degenerate Seg at (0, 0)
		 */
		explicit constexpr Seg();

		/**
		 * \brief Creates a Seg that starts and ends at the given points
		 */
		explicit Seg(const Point& start, const Point& end);

		/**
		 * \breif Returns the slope of this Seg, where 0 is horizontal and +NaN is vertical
		 */
		double slope() const;

		/**
		 * \brief Returns whether this Seg is degenerate, that is, whether the distance between the points is less than EPS.
		 */
		bool degenerate() const;

		/**
		 * \brief Returns the vector that points from the start to the end. This is equivalent to (end - start).
		 */
		Vector2 to_vector() const;

		/**
		 * \brief Returns this line segment as an infinite line that passes through this segment's start and end points.
		 */
		Line to_line() const;

		/**
		 * \brief Returns the length of this segment.
		 */
		double len() const;

		/**
		 * \brief Returns the length squared of this segment.
		 */
		double lensq() const;
	};
}
