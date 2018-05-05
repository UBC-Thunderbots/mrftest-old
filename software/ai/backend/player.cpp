#include "ai/backend/player.h"
#include <glibmm/ustring.h>
#include <cmath>
#include <iostream>
#include "geom/angle.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/param.h"

namespace
{
BoolParam kalman_control_inputs(
    u8"Enable Kalman control inputs", u8"AI/Backend", false);
}

using AI::BE::Player;

void Player::flags(AI::Flags::MoveFlags flags)
{
    if ((flags & ~AI::Flags::FLAGS_VALID) != AI::Flags::MoveFlags::NONE)
    {
        LOG_ERROR(Glib::ustring::compose(
            u8"Invalid flag(s) 0x%1",
            Glib::ustring::format(
                std::hex,
                static_cast<uint64_t>(flags & ~AI::Flags::FLAGS_VALID))));
        flags &= AI::Flags::FLAGS_VALID;
    }
    flags_ = flags;
}

bool Player::has_display_path() const
{
    return true;
}

const std::vector<Point>& Player::display_path() const
{
    return display_path_;
}

void Player::display_path(const std::vector<Point>& p)
{
    display_path_ = p;
}

void Player::pre_tick()
{
    AI::BE::Robot::pre_tick();
    flags_     = AI::Flags::MoveFlags::NONE;
    move_prio_ = AI::Flags::MovePrio::MEDIUM;
}

void Player::update_predictor(AI::Timestamp ts)
{
    if (kalman_control_inputs)
    {
#warning This needs writing for movement primitives.
    }
}

Visualizable::Colour Player::visualizer_colour() const
{
    return Visualizable::Colour(0.0, 1.0, 0.0);
}

bool Player::highlight() const
{
    return has_ball();
}

void Player::push_prim(AI::BE::Primitives::Ptr prim)
{
    // limit prims_ size to 1
    prims_.clear();

    prims_.push_back(prim);
}

void Player::clear_prims()
{
    prims_.clear();
}

void Player::erase_prim(AI::BE::Primitives::Ptr prim)
{
    if (prims_.size() <= 0)
    {
        return;
    }

    if (prims_.back() == prim)
    {
        prims_.pop_back();
    }
    else if (std::find(prims_.begin(), prims_.end(), prim) == prims_.end())
    {
        // primitive is already done, do nothing
        return;
    }
    else
    {
        // Control has entered an illegal state - the primitive erased must be
        // at the back of the queue, or it must be already out of the queue!
        std::abort();
    }
}

void Player::pop_prim()
{
    if (has_prim())
    {
        prims_.pop_front();
    }
    else
    {
        std::abort();
    }
}

AI::BE::Primitives::Ptr Player::top_prim() const
{
    if (prims_.size() > 0)
    {
        return prims_.front();
    }
    return nullptr;
}

bool Player::has_prim() const
{
    return prims_.size() > 0;
}

Visualizable::Colour Player::highlight_colour() const
{
    if (has_ball())
    {
        return Visualizable::Colour(1.0, 0.5, 0.0);
    }
    else
    {
        return Visualizable::Colour(0.0, 0.0, 0.0);
    }
}

Player::Player(unsigned int pattern)
    : AI::BE::Robot(pattern),
      flags_(AI::Flags::MoveFlags::NONE),
      move_prio_(AI::Flags::MovePrio::MEDIUM)
{
}
