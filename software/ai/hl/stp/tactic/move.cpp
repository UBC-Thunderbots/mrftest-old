#include <algorithm>

#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/util.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;

namespace
{
class MoveOnce final : public Tactic
{
   public:
    explicit MoveOnce(World world, Point dest, Angle orientation)
        : Tactic(world),
          dest(dest),
          orientation(orientation)  // MoveOnce with final orientation
    {
        bHasOrientation = 1;
    }
    explicit MoveOnce(World world, Point dest)
        : Tactic(world),
          dest(dest)  // MoveOnce with no preset final orientation
    {
        bHasOrientation = 0;
    }

    // REFERENCE: Tactic superclass for override methods descriptions.

   private:
    const Point dest;
    const Angle orientation;
    bool bHasOrientation;  // boolean that controls the version of overloaded
                           // move used (depends on if the function called
                           // specifies an orientation)

    Player select(const std::set<Player>& players) const override;

    void execute(caller_t& caller) override;

    Glib::ustring description() const override
    {
        return u8"move-once";
    }
};

Player MoveOnce::select(const std::set<Player>& players) const
{
    return *std::min_element(
        players.begin(), players.end(),
        AI::HL::Util::CmpDist<Player>(
            dest));  // returns first element/robot from player vector
}

// executes caller on selected player

void MoveOnce::execute(caller_t& caller)
{
    while (true)
    {
        if (bHasOrientation)
        {
            Action::move(
                caller, world, player(), dest,
                orientation);  // if an orientation has been specified, then use
                               // the overloaded move action with orientation
            caller();
        }
        else
        {
            Action::move(caller, world, player(), dest);
            caller();
        }
    }
}

class Move final : public Tactic
{
   public:
    explicit Move(World world, Point dest) : Tactic(world), dest(dest)
    {
        bHasOrientation = 0;
    }
    explicit Move(World world, Point dest, const Angle orientation)
        : Tactic(world), dest(dest), orientation(orientation)
    {
        bHasOrientation = 1;
    }

   private:
    const Point dest;
    const Angle orientation;
    bool bHasOrientation;

    Player select(const std::set<Player>& players) const override;

    void execute(caller_t& caller) override;

    Glib::ustring description() const override
    {
        return u8"move";
    }
};

Player Move::select(const std::set<Player>& players) const
{
    return *std::min_element(
        players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest));
}

void Move::execute(caller_t& caller)
{
    while (true)
    {
        if (bHasOrientation)
        {
            Action::move(caller, world, player(), dest, orientation, true);
            yield(caller);
        }
        else
        {
            Action::move(caller, world, player(), dest, true);
            yield(caller);
        }
    }
}
}

Tactic::Ptr AI::HL::STP::Tactic::move_once(World world, Point dest)
{
    return Tactic::Ptr(new MoveOnce(world, dest));
}

Tactic::Ptr AI::HL::STP::Tactic::move_once(
    World world, Point dest, Angle orientation)
{
    return Tactic::Ptr(new MoveOnce(world, dest, orientation));
}

Tactic::Ptr AI::HL::STP::Tactic::move(
    World world, Point dest, Angle orientation)
{
    return Tactic::Ptr(new Move(world, dest, orientation));
}

Tactic::Ptr AI::HL::STP::Tactic::move(World world, Point dest)
{
    return Tactic::Ptr(new Move(world, dest));
}
