#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/indirect_cherry.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

namespace {


}
Point AI::HL::STP::Evaluation::cherry_pivot(World world, Player player) {
	double distance = 100;
	Point destination = Point();
		for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
			int best_offender;
			Player test = world.friendly_team().get(i);
			double player_distance = (test.position() - world.field().enemy_goal()).len();
			if (player_distance < distance) {
				distance = player_distance;
				best_offender = i;
			}

			Player cherry_picker = world.friendly_team().get(best_offender);
			double magnitude = (world.ball().position() - cherry_picker.position()).lensq() + 0.15;
			Point norm = (world.ball().position() - cherry_picker.position()).norm();
			norm*=magnitude;
			return destination = world.ball().position() + norm;
		}

}

bool AI::HL::STP::Evaluation::cherry_at_point(World world, Player player) {
	return ((player.position() - cherry_pivot(world, player)).lensq() < Robot::MAX_RADIUS);
}



