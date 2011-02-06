#include "ai/hl/stp/evaluate/offense.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"

using namespace AI::HL::W;

namespace {
	const int GRID_SIZE = 25;

	const double DEG_2_RAD = 1.0 / 180.0 * M_PI;

	// avoid enemy robots by at least this distance
	const double NEAR = Robot::MAX_RADIUS * 3;

};

