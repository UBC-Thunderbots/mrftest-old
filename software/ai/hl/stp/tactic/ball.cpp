#include "ai/hl/stp/tactic/ball.h"

#include "ai/hl/stp/tactic/util.h"

#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/move_spin.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"

#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action     = AI::HL::STP::Action;
using AI::HL::STP::Coordinate;
using AI::HL::STP::Region;
using namespace std;

namespace
{
class SpinSteal final : public Tactic
{
   public:
    explicit SpinSteal(World world) : Tactic(world), none(false)
    {
    }

   private:
    bool none;
    bool done() const override;
    Player select(const std::set<Player>& players) const override;
    void execute(caller_t& ca) override;

    Glib::ustring description() const override
    {
        return u8"spin-steal";
    }
};

class BackUpSteal final : public Tactic
{
   public:
    explicit BackUpSteal(World world)
        : Tactic(world),
          state(BACKING_UP),
          finished(false),
          backup_dist(4 * Robot::MAX_RADIUS)
    {
    }

   private:
    enum state
    {
        BACKING_UP,
        GOING_FORWARD
    };

    state state;
    bool finished;
    Point start_pos;
    const double backup_dist;

    bool done() const override;
    Player select(const std::set<Player>& players) const override;
    void player_changed() override;
    void execute(caller_t& ca) override;

    Glib::ustring description() const override
    {
        return u8"backup-steal";
    }
};

class TActiveDef final : public Tactic
{
   public:
    explicit TActiveDef(World world) : Tactic(world), finished(false)
    {
    }

   private:
    bool finished;
    bool done() const override;
    Player select(const std::set<Player>& players) const override;
    void execute(caller_t& ca) override;

    Glib::ustring description() const override
    {
        return u8"tactive-def";
    }
};

class TDribbleToRegion final : public Tactic
{
   public:
    explicit TDribbleToRegion(World world, Region region_)
        : Tactic(world), region(region_)
    {
    }

   private:
    Region region;
    bool done() const override;
    Player select(const std::set<Player>& players) const override;
    void execute(caller_t& ca) override;

    Glib::ustring description() const override
    {
        return u8"tdribble-to-region";
    }
};

class TSpinToRegion final : public Tactic
{
   public:
    explicit TSpinToRegion(World world, Region region_)
        : Tactic(world), region(region_)
    {
    }

   private:
    Region region;
    bool done() const override;
    Player select(const std::set<Player>& players) const override;
    void execute(caller_t& ca) override;

    Glib::ustring description() const override
    {
        return u8"tspin-to-region";
    }
};

bool SpinSteal::done() const
{
    return none;
}

Player SpinSteal::select(const std::set<Player>& players) const
{
    return select_baller(world, players, player());
}

void SpinSteal::execute(caller_t& ca)
{
    while (true)
    {
        none            = false;
        EnemyTeam enemy = world.enemy_team();
        Point dirToBall =
            (world.ball().position() - player().position()).norm();
        for (const Robot i : enemy)
        {
            if (Evaluation::possess_ball(world, i))
            {
                Action::move_spin(
                    ca, player(),
                    world.ball().position() + Robot::MAX_RADIUS * dirToBall,
                    Angle::half());
                break;
            }
        }
        yield(ca);
        //			none = true;
    }
}

bool BackUpSteal::done() const
{
    return finished && player().has_ball();
}

Player BackUpSteal::select(const std::set<Player>& players) const
{
    return select_baller(world, players, player());
}

void BackUpSteal::player_changed()
{
    start_pos = player().position();
}

void BackUpSteal::execute(caller_t& ca)
{
    while (true)
    {
        finished = (player().position() - start_pos).len() > backup_dist;

        switch (state)
        {
            case BACKING_UP:
                Action::move(
                    ca, world, player(), start_pos + Point(-backup_dist, 0));
                if (finished)
                {
                    state = GOING_FORWARD;
                }
                break;
            case GOING_FORWARD:
                Action::move(ca, world, player(), world.ball().position());
                if (player().has_ball())
                {
                    state = BACKING_UP;
                }
                break;
        }

        yield(ca);
    }
}

bool TActiveDef::done() const
{
    return finished;
}

Player TActiveDef::select(const std::set<Player>& players) const
{
    return select_baller(world, players, player());
}

void TActiveDef::execute(caller_t& ca)
{
    while (true)
    {
        finished          = false;
        EnemyTeam enemy   = world.enemy_team();
        bool enemyHasBall = false;

        Point dirToBall = Point();
        for (const Robot i : enemy)
        {
            if (Evaluation::possess_ball(world, i))
            {
                dirToBall    = (world.ball().position() - i.position()).norm();
                enemyHasBall = true;
                break;
            }
        }

        if (enemyHasBall)
        {
            Action::move_spin(
                ca, player(),
                world.ball().position() + 0.75 * Robot::MAX_RADIUS * dirToBall,
                Angle::half());
        }
        else
        {
            finished = Action::repel(ca, world, player());
        }

        yield(ca);
    }
}

bool TDribbleToRegion::done() const
{
    return region.inside(player().position());
}

Player TDribbleToRegion::select(const std::set<Player>& players) const
{
    return select_baller(world, players, player());
}

void TDribbleToRegion::execute(caller_t& ca)
{
    while (true)
    {
        Action::dribble(ca, world, player(), region.center_position());
    }
}

bool TSpinToRegion::done() const
{
    return region.inside(player().position());
}

Player TSpinToRegion::select(const std::set<Player>& players) const
{
    return select_baller(world, players, player());
}

void TSpinToRegion::execute(caller_t& ca)
{
    Action::move_spin(ca, player(), region.center_position(), Angle::half());
}
}

Tactic::Ptr AI::HL::STP::Tactic::spin_steal(World world)
{
    Tactic::Ptr p(new SpinSteal(world));
    return p;
}

Tactic::Ptr AI::HL::STP::Tactic::back_up_steal(World world)
{
    Tactic::Ptr p(new BackUpSteal(world));
    return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tactive_def(World world)
{
    Tactic::Ptr p(new TActiveDef(world));
    return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdribble_to_region(World world, Region region_)
{
    Tactic::Ptr p(new TDribbleToRegion(world, region_));
    return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tspin_to_region(World world, Region region_)
{
    Tactic::Ptr p(new TSpinToRegion(world, region_));
    return p;
}
