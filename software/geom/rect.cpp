#include "geom/rect.h"
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

void Rect::translate(const Point &offset) {
	min_corner += offset;
}

