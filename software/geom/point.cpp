#include "geom/point.h"
#include <cmath>

using namespace std;

point rotate(const point &pt, double rot) {
	return pt * point(cos(rot), sin(rot));
}

