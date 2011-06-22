#ifndef GEOM_POINT_H
#define GEOM_POINT_H

#include <cmath>
#include <ostream>

/**
 * A point or vector in 2D space.
 */
class Point {
	public:
		/**
		 * The X coordinate of the Point.
		 */
		double x;

		/**
		 * The Y coordinate of the Point.
		 */
		double y;

		/**
		 * Creates a unit-magnitude Point for an angle.
		 *
		 * \param[in] angle the angle.
		 *
		 * \return the Point.
		 */
		static Point of_angle(double angle);

		/**
		 * Creates the origin.
		 */
		Point();

		/**
		 * Creates a Point at arbitrary coordinates.
		 *
		 * \param[in] x the <var>x</var> value of the Point.
		 *
		 * \param[in] y the <var>y</var> value of the Point.
		 */
		Point(double x, double y);

		/**
		 * Creates a copy of a Point.
		 *
		 * \param[in] p the Point to duplicate.
		 */
		Point(const Point &p);

		/**
		 * Returns the square of the length of the Point.
		 *
		 * \return the square of the length of the Point.
		 */
		double lensq() const __attribute__((warn_unused_result));

		/**
		 * Returns the length of the Point.
		 *
		 * \return the length of the Point.
		 */
		double len() const __attribute__((warn_unused_result));

		/**
		 * Returns the unit vector in the same direction as this Point.
		 *
		 * \return a unit vector in the same direction as this Point, or a zero-length Point if this Point is zero.
		 */
		Point norm() const __attribute__((warn_unused_result));

		/**
		 * Returns a scaled normalized vector in the same direction as this Point.
		 *
		 * \param[in] l the desired length of the resultant vector.
		 *
		 * \return a vector in the same direction as this Point and with length \p l, or a zero-length Point if this Point is zero.
		 */
		Point norm(double l) const __attribute__((warn_unused_result));

		/**
		 * Returns the vector perpendicular to this Point.
		 *
		 * \return a vector perpendicular to this Point.
		 */
		Point perp() const __attribute__((warn_unused_result));

		/**
		 * Rotates this Point by an angle.
		 *
		 * \param[in] rot the angle in radians to rotate the vector.
		 *
		 * \return the Point rotated by rot.
		 */
		Point rotate(double rot) const __attribute__((warn_unused_result));

		/**
		 * Projects this vector onto another vector.
		 *
		 * \param[in] n the vector to project onto.
		 *
		 * \return the component of \p this that is in the same direction as \p n.
		 */
		Point project(const Point &n) const __attribute__((warn_unused_result));

		/**
		 * Takes the dot product of two vectors.
		 *
		 * \param[in] other the Point to dot against.
		 *
		 * \return the dot product of the points.
		 */
		double dot(const Point &other) const __attribute__((warn_unused_result));

		/**
		 * Takes the cross product of two vectors.
		 *
		 * \param[in] other the Point to cross with.
		 *
		 * \return the <var>z</var> component of the 3-dimensional cross product \p *this × \p other.
		 */
		double cross(const Point &other) const __attribute__((warn_unused_result));

		/**
		 * Assigns one vector to another.
		 *
		 * \param[in] q the vector whose value should be copied into this vector.
		 *
		 * \return this vector.
		 */
		Point &operator=(const Point &q);

		/**
		 * Returns the direction of this vector.
		 *
		 * \return the direction of this vector, in the range [-π, π], with 0 being the positive <var>x</var> direction, π/2 being up, etc.
		 * (in actuality, this is <code>std::atan2(y, x)</code>).
		 */
		double orientation() const __attribute__((warn_unused_result));
};

/**
 * Adds two points.
 *
 * \param[in] p the first Point.
 *
 * \param[in] q the second Point.
 *
 * \return the vector-sum of the two points.
 */
Point operator+(const Point &p, const Point &q) __attribute__((warn_unused_result));

/**
 * Adds an offset to a Point.
 *
 * \param[in,out] p the Point to add the offset to.
 *
 * \param[in] q the offset to add.
 *
 * \return \p p.
 */
Point &operator+=(Point &p, const Point &q);

/**
 * Negates a Point.
 *
 * \param[in] p the Point to negate.
 *
 * \return \c −p.
 */
Point operator-(const Point &p) __attribute__((warn_unused_result));

/**
 * Subtracts two points.
 *
 * \param[in] p the Point to subtract from.
 *
 * \param[in] q the Point to subtract.
 *
 * \return the vector-difference of the two points.
 */
Point operator-(const Point &p, const Point &q) __attribute__((warn_unused_result));

/**
 * Subtracts an offset from a Point.
 *
 * \param[in,out] p the Point to subtract the offset from.
 *
 * \param[in] q the offset to subtract.
 *
 * \return p.
 */
Point &operator-=(Point &p, const Point &q);

/**
 * Multiplies a vector by a scalar.
 *
 * \param[in] s the scaling factor.
 *
 * \param[in] p the vector to scale.
 *
 * \return the scaled vector.
 */
Point operator*(double s, const Point &p) __attribute__((warn_unused_result));

/**
 * Multiplies a vector by a scalar.
 *
 * \param[in] p the vector to scale.
 *
 * \param[in] s the scaling factor.
 *
 * \return the scaled vector.
 */
Point operator*(const Point &p, double s) __attribute__((warn_unused_result));

/**
 * Scales a vector by a scalar.
 *
 * \param[in,out] p the vector to scale.
 *
 * \param[in] s the scaling factor.
 *
 * \return \p p.
 */
Point &operator*=(Point &p, double s);

/**
 * Divides a vector by a scalar.
 *
 * \param[in] p the vector to scale.
 *
 * \param[in] s the scalar to divide by.
 *
 * \return the scaled vector.
 */
Point operator/(const Point &p, double s) __attribute__((warn_unused_result));

/**
 * Scales a vector by a scalar.
 *
 * \param[in,out] p the vector to scale.
 *
 * \param[in] s the sclaing factor.
 *
 * \return \p p.
 */
Point &operator/=(Point &p, double s);

/**
 * Prints a vector to a stream.
 *
 * \param[in] os the stream to print to.
 *
 * \param[in] p the Point to print.
 *
 * \return \p os.
 */
std::ostream &operator<<(std::ostream &os, const Point &p);

/**
 * Orders two vectors suitably for sorting.
 * Vectors are ordered first by <var>x</var> coordinate and then by <var>y</var> coordinate.
 *
 * \param[in] p the first Point.
 *
 * \param[in] q the second Point.
 *
 * \return \c true if \p p < \p q, or \c false otherwise.
 */
bool operator<(const Point &p, const Point &q);

inline Point Point::of_angle(double angle) {
	return Point(std::cos(angle), std::sin(angle));
}

inline Point::Point() : x(0.0), y(0.0) {
}

inline Point::Point(double x, double y) : x(x), y(y) {
}

inline Point::Point(const Point &p) : x(p.x), y(p.y) {
}

inline double Point::lensq() const {
	return x * x + y * y;
}

inline double Point::len() const {
	return std::sqrt(lensq());
}

inline Point Point::norm() const {
	const double l = len();
	if (l < 1.0e-9) {
		return Point();
	} else {
		return Point(x / l, y / l);
	}
}

inline Point Point::norm(double l) const {
	double nl = l / len();
	if (len() < 1.0e-9) {
		return Point();
	} else {
		return Point(x * nl, y * nl);
	}
}

inline Point Point::perp() const {
	return Point(-y, x);
}

inline Point Point::rotate(double rot) const {
	double s = std::sin(rot);
	double c = std::cos(rot);
	return Point(x * c - y * s, x * s + y * c);
}

inline Point Point::project(const Point &n) const {
	return dot(n) / n.lensq() * n;
}

inline double Point::dot(const Point &other) const {
	return x * other.x + y * other.y;
}

inline double Point::cross(const Point &other) const {
	return x * other.y - y * other.x;
}

inline Point &Point::operator=(const Point &q) {
	x = q.x;
	y = q.y;
	return *this;
}

inline double Point::orientation() const {
	return std::atan2(y, x);
}

inline Point operator+(const Point &p, const Point &q) {
	return Point(p.x + q.x, p.y + q.y);
}

inline Point &operator+=(Point &p, const Point &q) {
	p.x += q.x;
	p.y += q.y;
	return p;
}

inline Point operator-(const Point &p) {
	return Point(-p.x, -p.y);
}

inline Point operator-(const Point &p, const Point &q) {
	return Point(p.x - q.x, p.y - q.y);
}

inline Point &operator-=(Point &p, const Point &q) {
	p.x -= q.x;
	p.y -= q.y;
	return p;
}

inline Point operator*(double s, const Point &p) {
	return Point(p.x * s, p.y * s);
}

inline Point operator*(const Point &p, double s) {
	return Point(p.x * s, p.y * s);
}

inline Point &operator*=(Point &p, double s) {
	p.x *= s;
	p.y *= s;
	return p;
}

inline Point operator/(const Point &p, double s) {
	return Point(p.x / s, p.y / s);
}

inline Point &operator/=(Point &p, double s) {
	p.x /= s;
	p.y /= s;
	return p;
}

inline std::ostream &operator<<(std::ostream &os, const Point &p) {
	os << '(' << p.x << ',' << p.y << ')';
	return os;
}

inline bool operator<(const Point &p, const Point &q) {
	if (p.x != q.x) {
		return p.x < q.x;
	} else {
		return p.y < q.y;
	}
}

#endif

