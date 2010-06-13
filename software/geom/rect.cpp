#include "./geom/rect.h"
#include <algorithm>
#include <cmath>


rect::rect(const point &point1, const point &point2):min_corner(std::min(point1.x, point2.x),std::min(point1.y, point2.y)),diag(fabs((point1 - point2).x), fabs((point1 - point2).y)) {
}

rect::rect(const point &sw_corner, double width, double height): min_corner(sw_corner),diag(width,height) {
}


double rect::width() const {
	return diag.x;
}

double rect::height() const {
	return diag.y;
}

double rect::area() const {
	return diag.x*diag.y;
}

point rect::centre() const {
	return min_corner + diag/2;
}

point rect::ne_corner() const {
	return min_corner + diag;
}

point rect::nw_corner() const {
	return min_corner + point(0,diag.y);
}

point rect::sw_corner() const {
	return min_corner;
}

point rect::se_corner() const {
	return min_corner + point(diag.x,0);
}

void rect::translate(const point &offset) {
	min_corner += offset;
}

