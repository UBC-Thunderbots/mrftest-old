#pragma once

#include <functional>
#include <utility>
#include <vector>
#include "ai/backend/backend.h"
#include "ai/backend/primitives/primitive.h"
#include "ai/common/world.h"
#include "ai/flags.h"

namespace AI
{
namespace Nav
{
class Waypoints
{
   public:
    typedef std::shared_ptr<Waypoints> Ptr;
    static constexpr std::size_t NUM_WAYPOINTS = 50;
    Point points[NUM_WAYPOINTS];
    AI::Flags::MoveFlags added_flags;
    Timestamp lastSentTime;
    Point move_dest;
};

namespace W
{
/**
 * \brief The field, as seen by a Navigator
 */
typedef AI::Common::Field Field;

/**
 * \brief The ball, as seen by a Navigator
 */
typedef AI::Common::Ball Ball;

/**
 * \brief A robot, as seen by a Navigator
 */
class Robot : public AI::Common::Robot
{
   public:
    /**
     * \brief The largest possible radius of a robot, in metres
     */
    static constexpr double MAX_RADIUS = 0.09;

    /**
     * \brief Constructs a nonexistent Robot
     */
    explicit Robot();

    /**
     * \brief Constructs a new Robot
     *
     * \param[in] impl the backend implementation to wrap
     */
    explicit Robot(AI::BE::Robot::Ptr impl);

    /**
     * \brief Constructs a copy of a Robot
     *
     * \param[in] copyref the object to copy
     */
    Robot(const Robot &copyref);

    bool replace(
        double x, double y, double dir, int id, bool is_yellow) override;

    /**
     * \brief Returns the avoidance distance for this robot
     *
     * \return the avoidance distance
     */
    AI::Flags::AvoidDistance avoid_distance() const;
};

class PlayerData
{
   public:
    inline PlayerData()
    {
    }

    typedef std::shared_ptr<PlayerData> Ptr;
    AI::Flags::MovePrio prev_move_prio;
    AI::Flags::AvoidDistance prev_avoid_distance;
    Point previous_dest;
    Angle previous_orient;

    AI::BE::Primitives::PrimitiveDescriptor last_shoot_primitive;
    AI::BE::Primitives::PrimitiveDescriptor hl_request;
    AI::BE::Primitives::PrimitiveDescriptor nav_request;
    bool fancy_shoot_maneuver;
};

/**
 * \brief A player, as seen by a Navigator
 */
class Player final : public AI::Common::Player, public Robot
{
   public:
    /**
     * \brief Planner waypoints
     */
    std::shared_ptr<Waypoints> waypoints;
    /**
     * \brief PlayerData for the navigator
     */
    std::shared_ptr<PlayerData> playerdata;

    /**
     * \brief The type of a complete path
     */
    typedef std::vector<std::pair<
        std::pair<Point, Angle>, std::chrono::steady_clock::time_point>>
        Path __attribute__((deprecated("This type should no longer be used.")));

    /**
     * \brief The maximum linear velocity of the robot, in metres per second
     */
    static constexpr double MAX_LINEAR_VELOCITY = 2;

    /**
     * \brief The maximum linear acceleration of the robot, in metres per second
     * squared
     */
    static constexpr double MAX_LINEAR_ACCELERATION = 3.5;

    /**
     * \brief The maximum angular velocity of the robot per second
     */
    static constexpr Angle MAX_ANGULAR_VELOCITY = Angle::of_radians(38);

    /**
     * \brief The maximum angular acceleration of the robot per second squared
     */
    static constexpr Angle MAX_ANGULAR_ACCELERATION = Angle::of_radians(77.7);

    /**
     * \brief Constructs a nonexistent Player
     */
    explicit Player();

    /**
     * \brief Constructs a new Player
     *
     * \param[in] impl the backend implementation to wrap
     */
    explicit Player(AI::BE::Player::Ptr impl);

    /**
     * \brief Constructs a copy of a Player
     *
     * \param[in] copyref the object to copy
     */
    Player(const Player &copyref);

    using AI::Common::Player::operator==;
    using AI::Common::Player::operator!=;
    using AI::Common::Player::operator bool;

    /**
     * \brief Returns the movement flags requested by the HighLevel
     *
     * \return the flags
     */
    AI::Flags::MoveFlags flags() const;

    /**
     * \brief Returns the movement priority requested by the HighLevel
     *
     * \return the priority
     */
    AI::Flags::MovePrio prio() const;

    /**
     * \brief Returns the current movement primitive
     *
     * \return the primitive
     */
    const Property<Drive::Primitive> &primitive() const;

    /**
     * \brief Sets the path this player should follow
     *
     * \param[in] p the path (an empty path causes the robot to halt)
     */
    void path(
        const std::vector<std::pair<
            std::pair<Point, Angle>, std::chrono::steady_clock::time_point>> &p)
        __attribute__((
            deprecated("You should use movement primitives instead.")));

    /**
     * \brief Sets the path this player plans to follow, for
     * display purposes.
     *
     * \param[in] p the path
     */
    void display_path(const std::vector<Point> &p);

    void push_prim(AI::BE::Primitives::Primitive *prim);

    void erase_prim(AI::BE::Primitives::Primitive *prim);

    void pop_prim();

    bool has_prim() const;

    AI::BE::Primitives::Primitive *top_prim() const;

    /**
     * \brief Coasts the robot’s wheels.
     */
    void move_coast();

    /**
     * \brief Brakes the robot’s wheels.
     */
    void move_brake();

    /**
     * \brief Moves the robot to a target position.
     *
     * The robot’s orientation upon reaching the target position is
     * unspecified.
     *
     * \param[in] dest the position to move to, as a distance forward
     * and left of the robot’s current position, relative to the
     * robot’s current orientation
     */
    void move_move(Point dest);

    /**
     * \brief Moves the robot to a target position and orientation.
     *
     * \param[in] dest the position to move to, as a distance forward
     * and left of the robot’s current position, relative to the
     * robot’s current orientation
     * \param[in] orientation how far left to rotate the robot to reach
     * its desired orientation
     */
    void move_move(Point dest, Angle orientation);

    /**
     * \brief Moves the robot to a target position.
     *
     * The robot’s orientation upon reaching the target position is
     * unspecified.
     *
     * \param[in] dest the position to move to, as a distance forward
     * and left of the robot’s current position, relative to the
     * robot’s current orientation
     * \param[in] time_delta how many seconds in the future the robot
     * should arrive at its destination
     */
    void move_move(Point dest, double time_delta);

    /**
     * \brief Moves the robot to a target position and orientation.
     *
     * \param[in] dest the position to move to, as a distance forward
     * and left of the robot’s current position, relative to the
     * robot’s current orientation
     * \param[in] orientation how far left to rotate the robot to reach
     * its desired orientation
     * \param[in] time_delta how many seconds in the future the robot
     * should arrive at its destination
     */
    void move_move(Point dest, Angle orientation, double time_delta);

    /**
     * \brief Moves the robot while carrying the ball.
     *
     * \param[in] dest the position to move to, as a distance forward
     * and left of the robot’s current position, relative to the
     * robot’s current orientation
     * \param[in] orientation how far left to rotate the robot to reach
     * its desired orientation
     * \param[in] small_kick_allowed whether or not the robot is
     * allowed to kick the ball ahead of itself while moving
     */
    void move_dribble(
        Point dest, Angle orientation, double desired_rpm,
        bool small_kick_allowed);

    /**
     * \brief Kicks the ball.
     *
     * The direction of the kick, and the robot’s final orientation,
     * are unspecified.
     *
     * \param[in] dest the position of the ball, as a distance forward
     * and left of the robot’s current position, relative to the
     * robot’s current orientation
     * \param[in] power how fast in m/s (for kicking) or how far in m
     * (for chipping) to kick the ball
     * \param[in] chip \c true to chip the ball or \c false to kick it
     */
    void __attribute__((optimize("O0")))
    move_shoot(Point dest, double power, bool chip);

    /**
     * \brief Kicks the ball.
     *
     * \param[in] dest the position of the ball, as a distance forward
     * and left of the robot’s current position, relative to the
     * robot’s current orientation
     * \param[in] orientation how far left the robot should rotate
     * before it kicks in order to kick the ball in the desired
     * direction
     * \param[in] power how fast in m/s (for kicking) or how far in m
     * (for chipping) to kick the ball
     * \param[in] chip \c true to chip the ball or \c false to kick it
     */
    void move_shoot(Point dest, Angle orientation, double power, bool chip);

    /**
     * \brief Catches a moving ball.
     *
     * \param[in] angle_diff how far left of the robot’s current
     * orientation would make it be pointing exactly 180° from the
     * ball’s current velocity (and thus pointing in the perfect
     * direction to receive the ball)
     * \param[in] displacement the distance to the left of the robot’s
     * current position to move to in order to be in the ball’s path,
     * where “left” is defined relative to the robot’s orientation
     * <em>after the requested rotation</em>
     * \param[in] speed the speed the robot should move forward (or
     * negative for backward), where “forward” is defined relative to
     * the robot’s orientation <em>after the requested rotation</em>
     */
    void move_catch(Angle angle_diff, double displacement, double speed);

    /**
     * \brief Rotates around a point on the field (e.g. the ball) while
     * facing it.
     *
     * Throughout the pivot, the robot maintains a fixed distance from
     * the centre point.
     *
     * \param[in] centre the position of the centre of the circle, as a
     * distance forward and left of the robot’s current position,
     * relative to the robot’s current orientation
     * \param[in] swing how far counterclockwise to swing the vector
     * from centre point to robot about the centre point to get the
     * final position of the robot
     * \param[in] orientation how far left the robot should rotate to
     * reach the desired final orientation
     */
    void move_pivot(Point centre, Angle swing, Angle orientation);

    /**
     * \brief Spins around rapidly while moving.
     *
     * \param[in] dest the position to move to, as a distance forward
     * and left of the robot’s current position, relative to the
     * robot’s current orientation
     * \param[in] speed the speed to spin at, in units per second
     */
    void move_spin(Point dest, Angle speed);
};

/**
 * \brief The friendly team
 */
typedef AI::Common::Team<Player, AI::BE::Player> FriendlyTeam;

/**
 * \brief The enemy team
 */
typedef AI::Common::Team<Robot, AI::BE::Robot> EnemyTeam;

/**
 * \brief The world, as seen by a Navigator
 */
class World final
{
   public:
    /**
     * \brief Constructs a new World
     *
     * \param[in] impl the backend implementation
     */
    explicit World(AI::BE::Backend &impl);

    /**
     * \brief Constructs a copy of a World
     *
     * \param[in] copyref the object to copy
     */
    World(const World &copyref);

    /**
     * \brief Returns the field
     *
     * \return the field
     */
    const Field &field() const;

    /**
     * \brief Returns the ball
     *
     * \return the ball
     */
    Ball ball() const;

    /**
     * \brief Returns the friendly team
     *
     * \return the friendly team
     */
    FriendlyTeam friendly_team() const;

    /**
     * \brief Returns the enemy team
     *
     * \return the enemy team
     */
    EnemyTeam enemy_team() const;

    /**
     * \brief Returns the current monotonic time
     *
     * Monotonic time is a way of representing "game time", which always moves
     * forward
     * Monotonic time is consistent within the game world, and may or may not be
     * linked to real time
     * A navigator should \em always use this function to retrieve monotonic
     * time!
     * The AI will not generally have any use for real time
     *
     * \return the current monotonic time
     */
    Timestamp monotonic_time() const;

   private:
    AI::BE::Backend &impl;
};
}
}
}

namespace std
{
/**
 * \brief Provides a total ordering of Robot objects so they can be stored in
 * STL ordered containers
 */
template <>
struct less<AI::Nav::W::Robot> final
{
   public:
    /**
     * \brief Compares two objects
     *
     * \param[in] x the first objects
     *
     * \param[in] y the second objects
     *
     * \return \c true if \p x should precede \p y in an ordered container, or
     * \c false if not
     */
    bool operator()(
        const AI::Nav::W::Robot &x, const AI::Nav::W::Robot &y) const;

   private:
    std::less<AI::Common::Robot> cmp;
};

/**
 * \brief Provides a total ordering of Player objects so they can be stored in
 * STL ordered containers
 */
template <>
struct less<AI::Nav::W::Player> final
{
   public:
    /**
     * \brief Compares two objects
     *
     * \param[in] x the first objects
     *
     * \param[in] y the second objects
     *
     * \return \c true if \p x should precede \p y in an ordered container, or
     * \c false if not
     */
    bool operator()(
        const AI::Nav::W::Player &x, const AI::Nav::W::Player &y) const;

   private:
    std::less<AI::Common::Player> cmp;
};
}

inline AI::Nav::W::Robot::Robot() = default;

inline AI::Nav::W::Robot::Robot(AI::BE::Robot::Ptr impl)
    : AI::Common::Robot(impl)
{
}

inline AI::Nav::W::Robot::Robot(const Robot &) = default;

inline AI::Flags::AvoidDistance AI::Nav::W::Robot::avoid_distance() const
{
    return impl->avoid_distance();
}

inline AI::Nav::W::Player::Player() : playerdata(std::make_shared<PlayerData>())
{
}

inline AI::Nav::W::Player::Player(AI::BE::Player::Ptr impl)
    : AI::Common::Player(impl),
      AI::Nav::W::Robot(impl),
      playerdata(std::make_shared<PlayerData>())
{
}

inline AI::Nav::W::Player::Player(const Player &) = default;

inline AI::Flags::MoveFlags AI::Nav::W::Player::flags() const
{
    return AI::Common::Player::impl->flags();
}

inline AI::Flags::MovePrio AI::Nav::W::Player::prio() const
{
    return AI::Common::Player::impl->prio();
}

inline const Property<Drive::Primitive> &AI::Nav::W::Player::primitive() const
{
    return AI::Common::Player::impl->primitive();
}

inline void AI::Nav::W::Player::path(
    const std::vector<std::pair<
        std::pair<Point, Angle>, std::chrono::steady_clock::time_point>> &)
{
}

inline void AI::Nav::W::Player::display_path(const std::vector<Point> &p)
{
    AI::Common::Player::impl->display_path(p);
}

inline void AI::Nav::W::Player::push_prim(AI::BE::Primitives::Primitive *prim)
{
    AI::Common::Player::impl->push_prim(prim);
}

inline void AI::Nav::W::Player::erase_prim(AI::BE::Primitives::Primitive *prim)
{
    AI::Common::Player::impl->erase_prim(prim);
}

inline void AI::Nav::W::Player::pop_prim()
{
    AI::Common::Player::impl->pop_prim();
}

inline bool AI::Nav::W::Player::has_prim() const
{
    return AI::Common::Player::impl->has_prim();
}

inline AI::BE::Primitives::Primitive *AI::Nav::W::Player::top_prim() const
{
    return AI::Common::Player::impl->top_prim();
}

inline void AI::Nav::W::Player::move_coast()
{
    AI::Common::Player::impl->move_coast();
}

inline void AI::Nav::W::Player::move_brake()
{
    AI::Common::Player::impl->move_brake();
}

inline void AI::Nav::W::Player::move_move(Point dest)
{
    AI::Common::Player::impl->move_move(dest);
}

inline bool AI::Nav::W::Robot::replace(
    double x, double y, double dir, int id, bool is_yellow)
{
    AI::Common::Robot::impl->is_replace = true;
    AI::Common::Robot::impl->replace(x, y, dir, id, is_yellow);
    return true;
}

inline void AI::Nav::W::Player::move_move(Point dest, Angle orientation)
{
    AI::Common::Player::impl->move_move(dest, orientation);
}

inline void AI::Nav::W::Player::move_move(Point dest, double time_delta)
{
    AI::Common::Player::impl->move_move(dest, time_delta);
}

inline void AI::Nav::W::Player::move_move(
    Point dest, Angle orientation, double time_delta)
{
    AI::Common::Player::impl->move_move(dest, orientation, time_delta);
}

inline void AI::Nav::W::Player::move_dribble(
    Point dest, Angle orientation, double desired_rpm, bool small_kick_allowed)
{
    AI::Common::Player::impl->move_dribble(
        dest, orientation, desired_rpm, small_kick_allowed);
}

inline void AI::Nav::W::Player::move_shoot(Point dest, double power, bool chip)
{
    AI::Common::Player::impl->move_shoot(dest, power, chip);
}

inline void AI::Nav::W::Player::move_shoot(
    Point dest, Angle orientation, double power, bool chip)
{
    AI::Common::Player::impl->move_shoot(dest, orientation, power, chip);
}

inline void AI::Nav::W::Player::move_catch(
    Angle angle_diff, double displacement, double speed)
{
    AI::Common::Player::impl->move_catch(angle_diff, displacement, speed);
}

inline void AI::Nav::W::Player::move_pivot(
    Point centre, Angle swing, Angle orientation)
{
    AI::Common::Player::impl->move_pivot(centre, swing, orientation);
}

inline void AI::Nav::W::Player::move_spin(Point dest, Angle speed)
{
    AI::Common::Player::impl->move_spin(dest, speed);
}

inline AI::Nav::W::World::World(AI::BE::Backend &impl) : impl(impl)
{
}

inline AI::Nav::W::World::World(const World &) = default;

inline const AI::Nav::W::Field &AI::Nav::W::World::field() const
{
    return impl.field();
}

inline AI::Nav::W::Ball AI::Nav::W::World::ball() const
{
    return AI::Common::Ball(impl.ball());
}

inline AI::Nav::W::FriendlyTeam AI::Nav::W::World::friendly_team() const
{
    return FriendlyTeam(impl.friendly_team());
}

inline AI::Nav::W::EnemyTeam AI::Nav::W::World::enemy_team() const
{
    return EnemyTeam(impl.enemy_team());
}

inline AI::Timestamp AI::Nav::W::World::monotonic_time() const
{
    return impl.monotonic_time();
}

inline bool std::less<AI::Nav::W::Robot>::operator()(
    const AI::Nav::W::Robot &x, const AI::Nav::W::Robot &y) const
{
    return cmp(x, y);
}

inline bool std::less<AI::Nav::W::Player>::operator()(
    const AI::Nav::W::Player &x, const AI::Nav::W::Player &y) const
{
    return cmp(x, y);
}
