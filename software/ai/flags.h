#pragma once

#include "ai/common/playtype.h"
#include "util/flags.h"

namespace AI
{
namespace Flags
{
// === IMPORTANT ===
// When adding a flag, remember to add its decoding to log/analyzer.cpp in
// MOVE_FLAG_MAPPING!
// Never reuse a numerical value or game logs will become ambiguous.
// Never remove a flag or the log analyzer will fail to compile.
// If a flag should not be used, mark it with __attribute__((deprecated)).
// The only exception to the above is if a log version change occurs and old
// flags can be compacted.
// === IMPORTANT ===

/**
 * Flags indicating how robots comply with game rules.
 * Flags are set by the Strategy and examined by the Navigator to determine
 * which potential paths are legal.
 */
enum class MoveFlags : uint64_t
{
    /**
     * No flags.
     */
    NONE = 0x0000,

    /**
     * Do not exit the play area of the field.
     */
    CLIP_PLAY_AREA = 0x0001,

    /**
     * Avoid the ball by 50cm as required by ball-out-of-play rules.
     */
    AVOID_BALL_STOP = 0x0002,

    /**
     * Avoid the ball very slightly, e.g. to orbit before touching the ball at a
     * kickoff or free kick.
     */
    AVOID_BALL_TINY = 0x0004,

    /**
     * Do not enter the friendly defence area.
     */
    AVOID_FRIENDLY_DEFENSE = 0x0008,

    /**
     * Stay at least 20cm outside the enemy defense area as required by
     * ball-entering-play rules.
     */
    AVOID_ENEMY_DEFENSE = 0x0010,

    /**
     * Do not enter the enemy's field half.
     */
    STAY_OWN_HALF = 0x0020,

    /**
     * Stay more than 40cm outside the penalty mark line as required for
     * non-kickers in penalty kick rules.
     */
    PENALTY_KICK_FRIENDLY = 0x0040,

    /**
     * Stay more than 40cm outside the penalty mark line as required for
     * non-goalies in penalty kick rules.
     */
    PENALTY_KICK_ENEMY = 0x0080,

    /**
     * \brief Drive carefully instead of quickly.
     */
    CAREFUL = 0x0100,

    /**
     * Avoid the ball by a medium amount, for use in regular navigation.
     */
    AVOID_BALL_MEDIUM = 0x0200,
};

GEN_BITWISE(MoveFlags, uint64_t)
/**
 * The union of all existent flags.
 */
constexpr MoveFlags FLAGS_VALID =
    MoveFlags::CLIP_PLAY_AREA | MoveFlags::AVOID_BALL_STOP |
    MoveFlags::AVOID_BALL_TINY | MoveFlags::AVOID_BALL_MEDIUM |
    MoveFlags::AVOID_FRIENDLY_DEFENSE | MoveFlags::AVOID_ENEMY_DEFENSE |
    MoveFlags::STAY_OWN_HALF | MoveFlags::PENALTY_KICK_FRIENDLY |
    MoveFlags::PENALTY_KICK_ENEMY | MoveFlags::CAREFUL;

/**
 * Returns the correct flags for a common player,
 * i.e. a player that is \em not a goalie or the taker of a free kick, kickoff,
 * or penalty kick.
 * The caller should modify the returned value for the goalie or kicker.
 *
 * \param[in] pt the current play type.
 *
 * \return the appropriate flags.
 */
MoveFlags calc_flags(AI::Common::PlayType pt);

/**
 * Movement priorities indicating which robots should give way so other robots
 * can drive in a straight line.
 */
enum class MovePrio
{
    /**
     * An important movement which should not be diverted, such as handling the
     * ball.
     */
    HIGH,

    /**
     * An ordinary everyday movement, such as getting into the open to receive a
     * pass.
     */
    MEDIUM,

    /**
     * An unimportant movement, such as moving to the other side of the field
     * for a long-term strategic purpose.
     */
    LOW,
};

/**
 * \brief Avoidance distances indicating by how far players should avoid an
 * enemy.
 */
enum class AvoidDistance
{
    /**
     * \brief The players can come quite close to the enemy.
     */
    SHORT,

    /**
     * \brief The players can come within a moderate distance of the enemy.
     *
     * This is the default value.
     */
    MEDIUM,

    /**
     * \brief The players must stay a long way away from the enemy.
     */
    LONG,
};
}
}

/**
 * \brief Compares two movement priorities.
 *
 * A priority precedes another priority if it is more important.
 *
 * \param[in] a the first priority.
 *
 * \param[in] b the second priority.
 *
 * \return \c true if \p a is more important than \p b.
 */
inline bool operator<(AI::Flags::MovePrio a, AI::Flags::MovePrio b)
{
    return static_cast<unsigned int>(a) < static_cast<unsigned int>(b);
}

/**
 * \brief Compares two movement priorities.
 *
 * A priority succedes another priority if it is less important.
 *
 * \param[in] a the first priority.
 *
 * \param[in] b the second priority.
 *
 * \return \c true if \p a is less important than \p b.
 */
inline bool operator>(AI::Flags::MovePrio a, AI::Flags::MovePrio b)
{
    return static_cast<unsigned int>(a) > static_cast<unsigned int>(b);
}

/**
 * \brief Compares two movement priorities.
 *
 * A priority precedes another priority if it is more important.
 *
 * \param[in] a the first priority.
 *
 * \param[in] b the second priority.
 *
 * \return \c true if \p a is more important than or equal to \p b.
 */
inline bool operator<=(AI::Flags::MovePrio a, AI::Flags::MovePrio b)
{
    return a < b || a == b;
}

/**
 * \brief Compares two movement priorities.
 *
 * A priority succedes another priority if it is less important.
 *
 * \param[in] a the first priority.
 *
 * \param[in] b the second priority.
 *
 * \return \c true if \p a is less important than or equal to \p b.
 */
inline bool operator>=(AI::Flags::MovePrio a, AI::Flags::MovePrio b)
{
    return a > b || a == b;
}
