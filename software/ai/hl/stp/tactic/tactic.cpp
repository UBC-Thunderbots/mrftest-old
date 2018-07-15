#include "ai/hl/stp/tactic/tactic.h"
#include <stdexcept>
#include "ai/hl/stp/action/action.h"
#include "util/dprint.h"
#include "util/param.h"

#include <iostream>

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::~Tactic() = default;

bool Tactic::done() const
{
    // this is probably a hack
    return false;
}

Player Tactic::player() const
{
    return player_;
}

void Tactic::player(Player p)
{
    if (player_ != p)
    {
        player_ = p;
        player_changed();
    }
}

void Tactic::tick()
{
    // Why do I have to check coroutine_ here as well?
    if (!this->done() && coroutine_)
    {
        coroutine_();
    }
}

void Tactic::draw_overlay(Cairo::RefPtr<Cairo::Context>) const
{
}

Tactic::Tactic(World world) : world(world)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"  // boost pls
    coroutine_ = coroutine_t([this](caller_t& caller) {
        caller();
        execute(caller);
    });
#pragma GCC diagnostic pop
}

void Tactic::player_changed()
{
    player_.clear_prims();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"  // boost pls
    coroutine_ = coroutine_t([this](caller_t& caller) {
        caller();
        execute(caller);
    });
#pragma GCC diagnostic pop
}

void Tactic::yield(caller_t& ca)
{
    Action::yield(ca);
}
