#pragma once

#include <functional>
#include "ai/backend/player.h"

namespace AI
{
namespace Common
{
class Player;
}
}

namespace std
{
/**
 * \brief Provides a total ordering of Player objects so they can be stored in
 * STL ordered containers
 */
template <>
struct less<AI::Common::Player> final
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
     * \c false if not.
     */
    bool operator()(
        const AI::Common::Player &x, const AI::Common::Player &y) const;

   private:
    std::less<BoxPtr<AI::BE::Player>> cmp;
};

template <>
struct hash<AI::Common::Player> final
{
   public:
    size_t operator()(const AI::Common::Player &x) const;
};
}

namespace AI
{
namespace Common
{
/**
 * \brief The common functions available on a player in all layers, not
 * including those in Robot
 */
class Player
{
   public:
    /**
     * \brief Constructs a nonexistent Robot
     */
    explicit Player();

    /**
     * \brief Constructs a new Robot
     *
     * \param[in] impl the backend implementation
     */
    explicit Player(AI::BE::Player::Ptr impl);

    /**
     * \brief Copies a Robot
     *
     * \param[in] copyref the object to copy
     */
    Player(const Player &copyref);

    /**
     * \brief Checks whether two robots are equal
     *
     * \param[in] other the robot to compare to
     *
     * \return \c true if the objects refer to the same robot, or \c false if
     * not
     */
    bool operator==(const Player &other) const;

    /**
     * \brief Checks whether two robots are equal
     *
     * \param[in] other the robot to compare to
     *
     * \return \c false if the objects refer to the same robot, or \c true if
     * not
     */
    bool operator!=(const Player &other) const;

    /**
     * \brief Checks whether the robot exists
     *
     * \return \c true if the object refers to an existing robot, or \c false if
     * not
     */
    explicit operator bool() const;

    /**
     * \brief Returns whether or not this player has the ball
     *
     * \return \c true if this player has the ball, or \c false if not
     */
    bool has_ball() const;

    /**
     * \brief Checks if this robot's chicker is ready to use
     *
     * \return \c true if ready, or \c false if not
     */
    bool chicker_ready() const;

    /**
     * \brief Checks if this robot has a chipper (a kicking device that can send
     * the ball up into the air)
     *
     * \return \c true if a chipper is available, or \c false if not
     */
    bool has_chipper() const;

    /**
     * \brief Checks if this robot's autokick or autochip mechanism fired in the
     * last tick
     *
     * \return \c true if the kicker was fired due to the autokick or autochip
     * mechanism since the last tick, or \c false if not
     */
    bool autokick_fired() const;

   protected:
    AI::BE::Player::Ptr impl;

    friend struct std::less<Player>;
    friend struct std::hash<Player>;
    friend class AI::BE::Primitives::Primitive;
};
}
}

inline bool std::less<AI::Common::Player>::operator()(
    const AI::Common::Player &x, const AI::Common::Player &y) const
{
    return cmp(x.impl, y.impl);
}

inline size_t std::hash<AI::Common::Player>::operator()(
    const AI::Common::Player &x) const
{
    return x.impl->pattern();
}

inline AI::Common::Player::Player() = default;

inline AI::Common::Player::Player(AI::BE::Player::Ptr impl) : impl(impl)
{
}

inline AI::Common::Player::Player(const Player &) = default;

inline bool AI::Common::Player::operator==(const Player &other) const
{
    return impl == other.impl;
}

inline bool AI::Common::Player::operator!=(const Player &other) const
{
    return !(*this == other);
}

inline AI::Common::Player::operator bool() const
{
    return !!impl;
}

inline bool AI::Common::Player::has_ball() const
{
    return impl->has_ball();
}

inline bool AI::Common::Player::chicker_ready() const
{
    return impl->chicker_ready();
}

inline bool AI::Common::Player::has_chipper() const
{
    return true;
}

inline bool AI::Common::Player::autokick_fired() const
{
    return impl->autokick_fired();
}