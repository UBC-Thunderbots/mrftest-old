#include "geom/point.h"

bool operator<(const Point &p, const Point &q) {
	if (p.x != q.x) {
		return p.x < q.x;
	} else {
		return p.y < q.y;
	}
}

