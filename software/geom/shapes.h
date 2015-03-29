#pragma once

#include "geom/point.h"
#include "geom/rect.h"

namespace Geom {
	class Seg final {
	public:
		Vector2 start;
		Vector2 end;

		/**
		 * \brief Creates a degenerate Seg at (0, 0)
		 */
		inline explicit constexpr Seg() { }

		/**
		 * \brief Creates a Seg that starts and ends at the given points
		 */
		inline explicit Seg(const Vector2& start, const Vector2& end) : start(start), end(end) { }
	};

	class Line final {
	public:
		Vector2 first;
		Vector2 second;

		/**
		 * \brief Creates a degenerate Line at (0, 0)
		 */
		inline explicit constexpr Line() { }

		/**
		 * \brief Creates a Seg that starts and ends at the given points
		 */
		inline explicit Line(const Vector2& first, const Vector2& second) : first(first), second(second) { }
	};

	class Circle final {
	public:
		Vector2 origin;
		double radius;

		/**
		 * \brief Creates a circle with radius 0 and radius 0.
		 */
		inline explicit constexpr Circle() : radius(0) { }

		inline explicit Circle(const Vector2& origin, double r) : origin(origin), radius(r) { }
	};
}
