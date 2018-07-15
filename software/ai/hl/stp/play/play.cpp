#include "ai/hl/stp/play/play.h"
#include "util/dprint.h"

using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayFactory;
using namespace AI::HL::W;

namespace
{
BoolParam test_goalie(u8"Override refbox goalie", u8"AI/HL/STP/Goalie", false);
BoolParam goalie_required(
    u8"Play can't execute without goalie", u8"AI/HL/STP/Goalie", false);

// One of the changes from the technical committee this year (2013) is that the
// referee box will send the pattern number of the goalie for each team.
// You can retrieve this with the goalie() function on a Team object.
// Therefore the following goalie code can only be used when the test_goalie
// param is turned on (for testing).
//
BoolParam goalie_lowest(u8"Goalie is lowest index", u8"AI/HL/STP/Goalie", true);
IntParam goalie_pattern_index(
    u8"Goalie pattern index", u8"AI/HL/STP/Goalie", 0, 0, 11);

std::vector<std::pair<Player, const AI::HL::STP::Tactic::Tactic*>>
    tactic_assignment;
}

namespace AI
{
namespace HL
{
namespace STP
{
namespace Play
{
namespace Global
{
const std::vector<std::pair<Player, const AI::HL::STP::Tactic::Tactic*>>&
get_tactic_assignment()
{
    return tactic_assignment;
}
}
}
}
}
}

Play::~Play() = default;

const std::array<AI::HL::STP::Tactic::Tactic::Ptr, AI::HL::STP::TEAM_MAX_SIZE>&
Play::get_tactics() const
{
    return tactics;
}

const std::array<AI::HL::W::Player, AI::HL::STP::TEAM_MAX_SIZE>&
Play::get_assignment() const
{
    return curr_assignment;
}

bool Play::can_give_up_safely() const
{
    return true;
}

void Play::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const
{
    for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i)
    {
        if (!curr_assignment[i])
        {
            continue;
        }

        tactics[i]->draw_overlay(ctx);
    }
}

void Play::wait(caller_t& ca, AI::HL::STP::Tactic::Tactic* tactic)
{
    while (!tactic->done())
    {
        yield(ca);
    }
}

void Play::yield(caller_t& ca)
{
    ca();
}

bool Play::role_assignment(std::vector<bool> players_enabled)
{
    // these must be reset every tick

    std::fill(curr_assignment.begin(), curr_assignment.end(), Player());

    Player goalie;
    // This removes the safety check of must having a goalie to execute a play.
    for (const Player p : world.friendly_team())
    {
        if (p.pattern() == world.friendly_team().goalie() &&
            players_enabled[p.pattern()])
        {
            goalie = p;
        }
    }

    if (test_goalie)
    {
        if (goalie_lowest)
        {
            goalie = world.friendly_team()[0];
        }
        else
        {
            for (const Player p : world.friendly_team())
            {
                if (p.pattern() ==
                        static_cast<unsigned int>(goalie_pattern_index) &&
                    players_enabled[p.pattern()])
                {
                    goalie = p;
                }
            }
        }

        if (!goalie && goalie_required)
        {
            LOG_ERROR(u8"No goalie with the desired pattern");
            return false;
        }
    }

    if (goalie)
    {
        assert(tactics[0]);
        tactics[0]->player(goalie);
        curr_assignment[0] = goalie;
    }
    else if (goalie_required)
    {
        LOG_ERROR(u8"No goalie with the desired pattern");
        return false;
    }

    // pool of available people
    std::set<Player> players;
    for (const Player p : world.friendly_team())
    {
        if ((goalie && p == goalie) || !players_enabled[p.pattern()])
        {
            continue;
        }
        players.insert(p);
    }

    for (std::size_t i = 1; i < TEAM_MAX_SIZE; ++i)
    {
        if (players.empty())
        {
            break;
        }

        if (!tactics[i])
            continue;

        // If the play is set to select players statically
        // and there is a previously saved assignment, then
        // use the previous saved assignment.
        if (this->factory().static_play && prev_assignment[i])
        {
            curr_assignment[i] = prev_assignment[i];
        }
        else
        {
            // let the tactic pick its player
            curr_assignment[i] = tactics[i]->select(players);
        }

        // assignment cannot be empty
        assert(curr_assignment[i]);
        assert(players.find(curr_assignment[i]) != players.end());
        players.erase(curr_assignment[i]);
        tactics[i]->player(curr_assignment[i]);
    }

    // save the current assignment and update globals
    tactic_assignment = decltype(tactic_assignment)();
    for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i)
    {
        prev_assignment[i] = curr_assignment[i];

        if (curr_assignment[i])
        {
            tactic_assignment.push_back(
                std::make_pair(curr_assignment[i], tactics[i].get()));
        }
    }

    return true;
}

bool Play::coroutine_finished() const
{
    return !_coroutine;
}

void Play::tick(const std::vector<bool>& players_enabled)
{
    // assign tactics to roles
    if (_coroutine)
    {
        _coroutine();
    }

    // assign roles to players
    if (!role_assignment(players_enabled))
    {
        return;
    }

    // execute!
    for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i)
    {
        if (!curr_assignment[i] || !tactics[i])
        {
            continue;
        }

        curr_assignment[i].flags(assignment_flags[i]);
        tactics[i]->tick();
        assignment_flags[i] = curr_assignment[i].flags();  // save flags
    }

    for (std::size_t i = 1; i < TEAM_MAX_SIZE; ++i)
    {
        if (!curr_assignment[i])
        {
            continue;
        }

        AI::Flags::MoveFlags extra_flags =
            AI::Flags::calc_flags(world.playtype());
        curr_assignment[i].flags(curr_assignment[i].flags() | extra_flags);
    }
}

Play::Play(World world) : world(world)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"  // boost pls
    _coroutine = coroutine_t([this](caller_t& caller) {
        caller();
        execute(caller);
    });
#pragma GCC diagnostic pop
    assignment_flags[0] = AI::Flags::MoveFlags::NONE;
    for (unsigned int i = 1; i < TEAM_MAX_SIZE; i++)
    {
        assignment_flags[i] = AI::Flags::MoveFlags::AVOID_FRIENDLY_DEFENSE;
    }
}

PlayFactory::PlayFactory(const char* name)
    : Registerable<PlayFactory>(name),
      enable(name, u8"AI/HL/STP/Play/Enable", true),
      priority(
          name, u8"AI/HL/STP/Play/Priority 0=low, 10=hi, 5=default", 5, 0, 10),
      playbook(name, u8"AI/HL/STP/Play/Playbook", 1023, 1, 1023),
      static_play(name, u8"AI/HL/STP/Play/StaticRoleAssignment", false)
{
}
