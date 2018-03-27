#include "ai/hl/stp/evaluation/indirect_cherry.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/move.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

Point AI::HL::STP::Evaluation::cherry_pivot(World world)
{
    std::vector<Player> players =
        AI::HL::Util::get_players(world.friendly_team());
    Point destination = Point();

    std::sort(
        players.begin(), players.end(),
        AI::HL::Util::CmpDist<Player>(world.ball().position()));

    Player cherry_picker = players[0];
    double magnitude =
        (world.ball().position() - cherry_picker.position()).len() + 0.15;
    Point norm = (world.ball().position() - cherry_picker.position()).norm();
    norm *= magnitude;
    return destination = world.ball().position() + norm;
}

bool AI::HL::STP::Evaluation::cherry_at_point(World world, Player player)
{
    return (
        (player.position() - cherry_pivot(world)).lensq() < Robot::MAX_RADIUS);
}
