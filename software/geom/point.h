#pragma once

#include "geom/angle.h"
#include <cmath>
#include <functional>
#include <ostream>

/**
 * \brief A point or vector in 2D space
 */
# warning make this Nullable
class Point final {
	public:
		/**
		 * \brief The X coordinate of the Point
		 */
		double x;

		/**
		 * \brief The Y coordinate of the Point
		 */
		double y;

		/**
		 * \brief Creates a unit-magnitude Point for an angle
		 *
		 * \param[in] angle the angle
		 *
		 * \return the Point
		 */
		static Point of_angle(Angle angle);

		/**
		 * \brief Creates the origin
		 */
		explicit constexpr Point();

		/**
		 * \brief Creates a Point at arbitrary coordinates.
		 *
		 * This can be used as an implicit type conersion from a braced initializer list.
		 * For example, <tt>{1, 2}</tt> can be passed to a function expecting a \c Point.
		 *
		 * \param[in] x the <var>x</var> value of the Point
		 *
		 * \param[in] y the <var>y</var> value of the Point
		 */
		constexpr Point(double x, double y);

		/**
		 * \brief Creates a copy of a Point
		 *
		 * \param[in] p the Point to duplicate
		 */
		constexpr Point(const Point &p);

		/**
		 * \brief Returns the square of the length of the Point
		 *
		 * \return the square of the length of the Point
		 */
		constexpr double lensq() const __attribute__((warn_unused_result));

		/**
		 * \brief Returns the length of the Point
		 *
		 * \return the length of the Point
		 */
		double len() const __attribute__((warn_unused_result));

		/**
		 * \brief Returns the unit vector in the same direction as this Point
		 *
		 * \return a unit vector in the same direction as this Point, or a zero-length Point if this Point is zero
		 */
		Point norm() const __attribute__((warn_unused_result));

		/**
		 * \brief Returns a scaled normalized vector in the same direction as this Point
		 *
		 * \param[in] l the desired length of the resultant vector
		 *
		 * \return a vector in the same direction as this Point and with length \p l, or a zero-length Point if this Point is zero
		 */
		Point norm(double l) const __attribute__((warn_unused_result));

		/**
		 * \brief Returns the vector perpendicular to this Point
		 *
		 * \return a vector perpendicular to this Point
		 */
		constexpr Point perp() const __attribute__((warn_unused_result));

		/**
		 * \brief Rotates this Point counterclockwise by an angle
		 *
		 * \param[in] rot the angle to rotate the vector
		 *
		 * \return the Point rotated by rot
		 */
		Point rotate(Angle rot) const __attribute__((warn_unused_result));

		/**
		 * \brief Projects this vector onto another vector
		 *
		 * \param[in] n the vector to project onto
		 *
		 * \return the component of \p this that is in the same direction as \p n
		 */
		constexpr Point project(const Point &n) const __attribute__((warn_unused_result));

		/**
		 * \brief Takes the dot product of two vectors
		 *
		 * \param[in] other the Point to dot against
		 *
		 * \return the dot product of the points
		 */
		constexpr double dot(const Point &other) const __attribute__((warn_unused_result));

		/**
		 * \brief Takes the cross product of two vectors
		 *
		 * \param[in] other the Point to cross with
		 *
		 * \return the <var>z</var> component of the 3-dimensional cross product \p *this × \p other
		 */
		constexpr double cross(const Point &other) const __attribute__((warn_unused_result));

		/**
		 * \brief Assigns one vector to another
		 *
		 * \param[in] q the vector whose value should be copied into this vector
		 *
		 * \return this vector
		 */
		Point &operator=(const Point &q);

		/**
		 * \brief Returns the direction of this vector
		 *
		 * \return the direction of this vector, in the range [-π, π], with 0 being the positive <var>x</var> direction, π/2 being up, etc.
		 * (in actuality, this is <code>std::atan2(y, x)</code>)
		 */
		Angle orientation() const __attribute__((warn_unused_result));

		/**
		 * \brief Checks whether this Point contains NaN in either coordinate
		 *
		 * \return \c true if either coordinate is NaN, or \c false if not
		 */
		constexpr bool isnan() const;

		/**
		 * \brief Checks whether this Point is close to another Point, where “close” is defined as 1.0e-9
		 *
		 * \param[in] other the other point to check against
		 */
		constexpr bool close(const Point &other) const;

		/**
		 * \brief Checks whether this Point is close to another Point
		 *
		 * \param[in] other the other point to check against
		 *
		 * \param[in] dist the distance to check against
		 */
		constexpr bool close(const Point &other, double dist) const;
};

namespace Geom {
	typedef Point Vector2;
}

/**
 * \brief Adds two points
 *
 * \param[in] p the first Point
 *
 * \param[in] q the second Point
 *
 * \return the vector-sum of the two points
 */
constexpr Point operator+(const Point &p, const Point &q) __attribute__((warn_unused_result));

/**
 * \brief Adds an offset to a Point
 *
 * \param[in,out] p the Point to add the offset to
 *
 * \param[in] q the offset to add
 *
 * \return \p p
 */
Point &operator+=(Point &p, const Point &q);

/**
 * \brief Negates a Point
 *
 * \param[in] p the Point to negate
 *
 * \return \c −p
 */
constexpr Point operator-(const Point &p) __attribute__((warn_unused_result));

/**
 * \brief Subtracts two points
 *
 * \param[in] p the Point to subtract from
 *
 * \param[in] q the Point to subtract
 *
 * \return the vector-difference of the two points
 */
constexpr Point operator-(const Point &p, const Point &q) __attribute__((warn_unused_result));

/**
 * \brief Subtracts an offset from a Point
 *
 * \param[in,out] p the Point to subtract the offset from
 *
 * \param[in] q the offset to subtract
 *
 * \return p
 */
Point &operator-=(Point &p, const Point &q);

/**
 * \brief Multiplies a vector by a scalar
 *
 * \param[in] s the scaling factor
 *
 * \param[in] p the vector to scale
 *
 * \return the scaled vector
 */
constexpr Point operator*(double s, const Point &p) __attribute__((warn_unused_result));

/**
 * \brief Multiplies a vector by a scalar
 *
 * \param[in] p the vector to scale
 *
 * \param[in] s the scaling factor
 *
 * \return the scaled vector
 */
constexpr Point operator*(const Point &p, double s) __attribute__((warn_unused_result));

/**
 * \brief Scales a vector by a scalar
 *
 * \param[in,out] p the vector to scale
 *
 * \param[in] s the scaling factor
 *
 * \return \p p
 */
Point &operator*=(Point &p, double s);

/**
 * \brief Divides a vector by a scalar
 *
 * \param[in] p the vector to scale
 *
 * \param[in] s the scalar to divide by
 *
 * \return the scaled vector
 */
constexpr Point operator/(const Point &p, double s) __attribute__((warn_unused_result));

/**
 * \brief Scales a vector by a scalar
 *
 * \param[in,out] p the vector to scale
 *
 * \param[in] s the sclaing factor
 *
 * \return \p p
 */
Point &operator/=(Point &p, double s);

/**
 * \brief Prints a vector to a stream
 *
 * \param[in] s the stream to print to
 *
 * \param[in] p the Point to print
 *
 * \return \p os
 */
template<typename CharT, typename Traits> std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &s, Point p);

/**
 * \brief Compares two vectors for equality
 *
 * \param[in] p the first Point
 *
 * \param[in] q the second Point
 *
 * \return \c true if \p p and \p q represent the same point, or \c false otherwise
 */
constexpr bool operator==(const Point &p, const Point &q);

/**
 * \brief Compares two vectors for inequality
 *
 * \param[in] p the first Point
 *
 * \param[in] q the second Point
 *
 * \return \c true if \p p and \p q represent different points, or \c false otherwise
 */
constexpr bool operator!=(const Point &p, const Point &q);

/**
 * \brief Orders two vectors suitably for sorting
 *
 * Vectors are ordered first by <var>x</var> coordinate and then by <var>y</var> coordinate.
 *
 * \param[in] p the first Point
 *
 * \param[in] q the second Point
 *
 * \return \c true if \p p < \p q, or \c false otherwise
 */
constexpr bool operator<(const Point &p, const Point &q);

/**
 * \brief Orders two vectors suitably for sorting
 *
 * Vectors are ordered first by <var>x</var> coordinate and then by <var>y</var> coordinate.
 *
 * \param[in] p the first Point
 *
 * \param[in] q the second Point
 *
 * \return \c true if \p p > \p q, or \c false otherwise
 */
constexpr bool operator>(const Point &p, const Point &q);

/**
 * \brief Orders two vectors suitably for sorting
 *
 * Vectors are ordered first by <var>x</var> coordinate and then by <var>y</var> coordinate.
 *
 * \param[in] p the first Point
 *
 * \param[in] q the second Point
 *
 * \return \c true if \p p ≤ \p q, or \c false otherwise
 */
constexpr bool operator<=(const Point &p, const Point &q);

/**
 * \brief Orders two vectors suitably for sorting
 *
 * Vectors are ordered first by <var>x</var> coordinate and then by <var>y</var> coordinate.
 *
 * \param[in] p the first Point
 *
 * \param[in] q the second Point
 *
 * \return \c true if \p p ≥ \p q, or \c false otherwise
 */
constexpr bool operator>=(const Point &p, const Point &q);





inline Point Point::of_angle(Angle angle) {
	return Point(angle.cos(), angle.sin());
}

inline constexpr Point::Point() : x(0.0), y(0.0) {
}

inline constexpr Point::Point(double x, double y) : x(x), y(y) {
}

inline constexpr Point::Point(const Point &p) : x(p.x), y(p.y) {
}

inline constexpr double Point::lensq() const {
	return x * x + y * y;
}

inline double Point::len() const {
	return std::hypot(x, y);
}

inline Point Point::norm() const {
	return len() < 1.0e-9 ? Point() : Point(x / len(), y / len());
}

inline Point Point::norm(double l) const {
	return len() < 1.0e-9 ? Point() : Point(x * l / len(), y * l / len());
}

inline constexpr Point Point::perp() const {
	return Point(-y, x);
}

inline Point Point::rotate(Angle rot) const {
	return Point(x * rot.cos() - y * rot.sin(), x * rot.sin() + y * rot.cos());
}

inline constexpr Point Point::project(const Point &n) const {
	return dot(n) / n.lensq() * n;
}

inline constexpr double Point::dot(const Point &other) const {
	return x * other.x + y * other.y;
}

inline constexpr double Point::cross(const Point &other) const {
	return x * other.y - y * other.x;
}

inline Point &Point::operator=(const Point &q) {
	x = q.x;
	y = q.y;
	return *this;
}

inline Angle Point::orientation() const {
	return Angle::of_radians(std::atan2(y, x));
}

inline constexpr bool Point::isnan() const {
	return x != x || y != y;
}

inline constexpr bool Point::close(const Point &other) const {
	return Point(x - other.x, y - other.y).lensq() < 1e-18;
}

inline constexpr bool Point::close(const Point &other, double dist) const {
	return Point(x - other.x, y - other.y).lensq() < dist * dist;
}

inline constexpr Point operator+(const Point &p, const Point &q) {
	return Point(p.x + q.x, p.y + q.y);
}

inline Point &operator+=(Point &p, const Point &q) {
	p.x += q.x;
	p.y += q.y;
	return p;
}

inline constexpr Point operator-(const Point &p) {
	return Point(-p.x, -p.y);
}

inline constexpr Point operator-(const Point &p, const Point &q) {
	return Point(p.x - q.x, p.y - q.y);
}

inline Point &operator-=(Point &p, const Point &q) {
	p.x -= q.x;
	p.y -= q.y;
	return p;
}

inline constexpr Point operator*(double s, const Point &p) {
	return Point(p.x * s, p.y * s);
}

inline constexpr Point operator*(const Point &p, double s) {
	return Point(p.x * s, p.y * s);
}

inline Point &operator*=(Point &p, double s) {
	p.x *= s;
	p.y *= s;
	return p;
}

inline constexpr Point operator/(const Point &p, double s) {
	return Point(p.x / s, p.y / s);
}

inline Point &operator/=(Point &p, double s) {
	p.x /= s;
	p.y /= s;
	return p;
}

template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &s, Point p) {
	s << '(' << p.x << ',' << p.y << ')';
	return s;
}

inline constexpr bool operator==(const Point &p, const Point &q) {
	return p.x == q.x && p.y == q.y;
}

inline constexpr bool operator!=(const Point &p, const Point &q) {
	return !(p == q);
}

inline constexpr bool operator<(const Point &p, const Point &q) {
	return p.x != q.x ? p.x < q.x : p.y < q.y;
}

inline constexpr bool operator>(const Point &p, const Point &q) {
	return !(p < q || p == q);
}

inline constexpr bool operator<=(const Point &p, const Point &q) {
	return p < q || p == q;
}

inline constexpr bool operator>=(const Point &p, const Point &q) {
	return !(p < q);
}

namespace std {
	template<> struct hash<Point> final {
		std::size_t operator()(const Point &p) const {
			std::hash<double> h;
			return h(p.x) * 17 + h(p.y);
		}
	};
}
