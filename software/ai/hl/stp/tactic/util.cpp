#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Player AI::HL::STP::Tactic::select_baller(
    World world, const std::set<Player> &players, Player previous)
{
    // if someone has ball, use it
    for (Player p : players)
    {
        if (p.has_ball())
        {
            return p;
        }
    }

    if (world.playtype() ==
            AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY ||
        world.playtype() ==
            AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY ||
        world.playtype() == AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY ||
        world.playtype() == AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY ||
        world.playtype() == AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY ||
        world.playtype() == AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY)
    {
        if (previous)
        {
            return previous;
        }
    }
    // closest
    Player best;
    double min_dist = 1e99;
    for (Player player : players)
    {
        Point dest = Evaluation::calc_fastest_grab_ball_dest(world, player);
        if (!best || min_dist > (dest - player.position()).len())
        {
            min_dist = (dest - player.position()).len();
            best     = player;
        }
    }

    return best;
}
