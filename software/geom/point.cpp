#include "geom/point.h"

bool operator<(const point &p, const point &q) {
	if (p.x != q.x) {
		return p.x < q.x;
	} else {
		return p.y < q.y;
	}
}

