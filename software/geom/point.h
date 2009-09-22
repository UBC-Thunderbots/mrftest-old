#ifndef GEOM_POINT_H
#define GEOM_POINT_H

#include <ostream>
#include <cmath>

//
// A point or vector in 2D space.
//
class point {
	public:
		//
		// The X coordinate of the point.
		//
		double x;

		//
		// The Y coordinate of the point.
		//
		double y;

		//
		// Creates a point at the origin.
		//
		point() : x(0.0), y(0.0) {
		}

		//
		// Creates a point at arbitrary coordinates.
		//
		point(double x, double y) : x(x), y(y) {
		}

		//
		// Creates a copy of a point.
		//
		point(const point &p) : x(p.x), y(p.y) {
		}

		//
		// Returns the square of the length of the vector.
		//
		double lensq() const {
			return x * x + y * y;
		}

		//
		// Returns the length of the vector.
		//
		double len() const {
			return std::sqrt(lensq());
		}

		//
		// Returns a rotation of the vector.
		//
		point rotate(double rot) const {
			return point(x * std::cos(rot) - y * std::sin(rot), x * std::sin(rot) + y * std::cos(rot));
		}

		//
		// Assigns one vector to another.
		//
		point &operator=(const point &q) {
			x = q.x;
			y = q.y;
			return *this;
		}
};

namespace {
	//
	// Adds two points.
	//
	point operator+(const point &p, const point &q) {
		return point(p.x + q.x, p.y + q.y);
	}

	//
	// Adds an offset to a point.
	//
	point &operator+=(point &p, const point &q) {
		p.x += q.x;
		p.y += q.y;
		return p;
	}

	//
	// Negates a point.
	//
	point operator-(const point &p) {
		return point(-p.x, -p.y);
	}

	//
	// Subtracts two points.
	//
	point operator-(const point &p, const point &q) {
		return point(p.x - q.x, p.y - q.y);
	}

	//
	// Subtracts an offset from a point.
	//
	point &operator-=(point &p, const point &q) {
		p.x -= q.x;
		p.y -= q.y;
		return p;
	}

	//
	// Multiplies a vector by a scalar.
	//
	point operator*(double s, const point &p) {
		return point(p.x * s, p.y * s);
	}

	//
	// Multiplies a vector by a scalar.
	//
	point operator*(const point &p, double s) {
		return point(p.x * s, p.y * s);
	}

	//
	// Scales a vector.
	//
	point &operator*=(point &p, double s) {
		p.x *= s;
		p.y *= s;
		return p;
	}

	//
	// Divides a vector by a scalar.
	//
	point operator/(const point &p, double s) {
		return point(p.x / s, p.y / s);
	}

	//
	// Scales a vector.
	//
	point &operator/=(point &p, double s) {
		p.x /= s;
		p.y /= s;
		return p;
	}

	//
	// Prints a vector.
	//
	std::ostream &operator<<(std::ostream &os, const point &p) {
		os << '(' << p.x << ',' << p.y << ')';
		return os;
	}
}

#endif

