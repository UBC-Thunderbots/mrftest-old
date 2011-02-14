#include "geom/rect.h"
#include "geom/util.h"
#include <algorithm>
#include <cmath>


Rect::Rect(const Point &point1, const Point &point2) : min_corner(std::min(point1.x, point2.x), std::min(point1.y, point2.y)), diag(fabs((point1 - point2).x), fabs((point1 - point2).y)) {
}

Rect::Rect(const Point &sw_corner, double width, double height) : min_corner(sw_corner), diag(width, height) {
}


double Rect::width() const {
	return diag.x;
}

double Rect::height() const {
	return diag.y;
}

double Rect::area() const {
	return diag.x * diag.y;
}

Point Rect::centre() const {
	return min_corner + diag / 2;
}

Point Rect::ne_corner() const {
	return min_corner + diag;
}

Point Rect::nw_corner() const {
	return min_corner + Point(0, diag.y);
}

Point Rect::sw_corner() const {
	return min_corner;
}

Point Rect::se_corner() const {
	return min_corner + Point(diag.x, 0);
}

bool Rect::point_inside(Point p) {
	return p.x >= min_corner.x && p.y >= min_corner.y && p.x <= min_corner.x + diag.x && p.y <= min_corner.y + diag.y;
}

bool Rect::expand(double amount) {
	if (diag.x < -2 * amount || diag.y < -2 * amount) {
		return false;
	}
	Point add(amount, amount);
	min_corner -= add;
	diag += 2 * add;
	return true;
}

double Rect::dist_to_boundary(Point p) {
	double dist = 10e9; // approx of infinity
	for (int i = 0; i < 4; i++) {
		Point a = operator[](i);
		Point b = operator[](i + 1);
		dist = std::min(dist, lineseg_point_dist(p, a, b));
	}
	return dist;
}

Point Rect::operator[](unsigned int pos) const {
	unsigned int temp = pos % 4;
	switch (temp) {
		case 1:
			return nw_corner();

		case 2:
			return ne_corner();

		case 3:
			return se_corner();
	}
	return sw_corner();
}

void Rect::translate(const Point &offset) {
	min_corner += offset;
}

