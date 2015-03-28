#pragma once

#include "geom/point.h"

namespace Geom {
	class Line final {
	public:
		Point first;
		Point second;

		/**
		 * \brief Creates a degenerate Line passing through (0, 0)
		 */
		explicit constexpr Line();

		/**
		 * \brief Creates a Line that passes through both points given
		 */
		explicit Line(const Point& first, const Point& second);

		/**
		 * \breif Returns the slope of this Line, where 0 is horizontal and +NaN is vertical
		 */
		double slope() const;

		/**
		 * \brief Returns whether this Line is degenerate, that is, whether the distance between the points is less than EPS.
		 */
		bool degenerate() const;
	};
}
