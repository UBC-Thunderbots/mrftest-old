#pragma once

#include <algorithm>
#include <utility>
#include <vector>
#include "ai/backend/robot.h"
// #include "ai/backend/primitives/primitive.h"
#include "ai/common/time.h"
#include "ai/flags.h"
#include "drive/robot.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/box_ptr.h"

namespace AI
{
namespace BE
{
namespace Primitives
{
class Primitive;
typedef std::shared_ptr<Primitive> Ptr;
}

/**
 * \brief A player, as exposed by the backend
 */
class Player : public AI::BE::Robot
{
   public:
    /**
     * \brief A pointer to a Player
     */
    typedef BoxPtr<Player> Ptr;

    /**
     * \brief The possible ways a robot can dribble.
     */
    enum class DribbleMode
    {
        /**
         * \brief The dribbler is off.
         */
        STOP,

        /**
         * \brief The dribbler is running to catch an incoming ball.
         */
        CATCH,

        /**
         * \brief The dribbler is running to pick up a loose ball.
         */
        INTERCEPT,

        /**
         * \brief The dribbler is running to carry a possessed ball.
         */
        CARRY,
    };

    AI::Flags::MoveFlags flags() const;
    void flags(AI::Flags::MoveFlags flags);
    void set_flags(AI::Flags::MoveFlags flags);
    void unset_flags(AI::Flags::MoveFlags flags);

    AI::Flags::MovePrio prio() const;
    void prio(AI::Flags::MovePrio prio);

    virtual const Property<Drive::Primitive>& primitive() const = 0;
    virtual void send_prim(Drive::LLPrimitive p)                = 0;
    bool has_display_path() const final override;
    const std::vector<Point>& display_path() const final override;
    void display_path(const std::vector<Point>& p);
    void pre_tick();
    void update_predictor(AI::Timestamp ts);

    void push_prim(Primitives::Ptr prim);
    void erase_prim(Primitives::Ptr prim);
    void pop_prim();
    void clear_prims();
    bool has_prim() const;
    Primitives::Ptr top_prim() const;

    Visualizable::Colour visualizer_colour() const final override;
    bool highlight() const final override;
    Visualizable::Colour highlight_colour() const final override;

    virtual bool has_ball() const                    = 0;
    virtual double get_lps(unsigned int index) const = 0;
    virtual bool chicker_ready() const               = 0;
    virtual bool autokick_fired() const              = 0;

   protected:
    explicit Player(unsigned int pattern);

   private:
    std::deque<Primitives::Ptr> prims_;
    AI::Flags::MoveFlags flags_;
    AI::Flags::MovePrio move_prio_;
    std::vector<Point> display_path_;
};
}
}

inline AI::Flags::MoveFlags AI::BE::Player::flags() const
{
    return flags_;
}

inline AI::Flags::MovePrio AI::BE::Player::prio() const
{
    return move_prio_;
}

inline void AI::BE::Player::prio(AI::Flags::MovePrio prio)
{
    move_prio_ = prio;
}
