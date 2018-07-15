#pragma once
#include <array>
#include <memory>
#include "geom/util.h"

namespace Drive
{
/**
 * \brief The movement primitive codes.
 */
enum class Primitive
{
    /**
     * \brief Implements the \ref Drive::Robot::move_coast and \ref
     * Drive::Robot::move_brake primitives.
     *
     * The parameters are unused.
     *
     * The extra field is 0 for coasting or 1 for braking.
     */
    STOP,

    /**
     * \brief Implements the \ref Drive::Robot::move_move family of
     * primitives.
     *
     * The parameters are the relative position, the relative orientation,
     * and the time delta.
     *
     * The extra field is 0 if the caller doesn’t care about orientation,
     * or 1 if it does.
     */
    MOVE,

    /**
     * \brief Implements the \ref Drive::Robot::move_dribble primitive.
     *
     * The parameters are the relative position and orientation.
     *
     * The extra field is 0 if small kicks are prohibited or 1 if they are
     * allowed.
     */
    DRIBBLE,

    /**
     * \brief Implements the \ref Drive::Robot::move_shoot family of
     * primitives.
     *
     * The parameters are the relative position, relative orientation, and
     * power (either m/s or m).
     *
     * The extra field has bit 0 clear to kick or set to chip, and bit 1
     * set if the caller cares about orientation.
     */
    SHOOT,

    /**
     * \brief Implements the \ref Drive::Robot::move_catch primitive.
     *
     * The parameters are the angle difference, the left/right
     * displacement, and the speed.
     */
    CATCH,

    /**
     * \brief Implements the \ref Drive::Robot::move_pivot primitive.
     *
     * The parameters are the relative centre point, the swing, and the
     * orientation.
     */
    PIVOT,

    /**
     * \brief Implements the \ref Drive::Robot::move_spin primitive.
     *
     * The parameters are the relative position and angular velocity.
     */
    SPIN,

    /**
     * \brief Specifies that direct control is in use and wheels are being
     * driven with individual power levels.
     */
    DIRECT_WHEELS,

    /**
     * \brief Specifies that direct control is in use and robot-relative
     * linear and angular velocities are being sent.
     */
    DIRECT_VELOCITY,
};

/**
 * \brief This class represents the movement primitive packet to be sent over
 * radio.
 */
class LLPrimitive
{
   public:
    LLPrimitive();
    LLPrimitive(Primitive prim, std::vector<double> p, uint8_t extra);

    static constexpr uint8_t PARAMS_MAX_SIZE = 4;

    Point field_point() const;
    Angle field_angle() const;

    Primitive prim;
    std::vector<double> params;
    uint8_t extra;
};
/**
 * \brief Coasts the robot’s wheels.
 */
LLPrimitive move_coast();

/**
 * \brief Brakes the robot’s wheels.
 */
LLPrimitive move_brake();

/**
 * \brief Moves the robot to a target position.
 *
 * The robot’s orientation upon reaching the target position is
 * unspecified.
 *
 * \param[in] dest the position to move to, as a distance forward
 * and left of the robot’s current position, relative to the
 * robot’s current orientation
 * \param[in] end_speed the speed (not velocity) that the robot
 * should attempt to be moving at when it reaches the destination
 */
LLPrimitive move_move(Point dest, double end_speed = 0.0, uint8_t extra = 0);

/**
 * \brief Moves the robot to a target position and orientation.
 *
 * \param[in] dest the position to move to, as a distance forward
 * and left of the robot’s current position, relative to the
 * robot’s current orientation
 * \param[in] orientation how far left to rotate the robot to reach
 * its desired orientation
 * \param[in] end_speed the speed (not velocity) that the robot
 * should attempt to be moving at when it reaches the destination
 */
LLPrimitive move_move(Point dest, Angle orientation, double end_speed = 0.0, uint8_t extra = 0);

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
LLPrimitive move_dribble(
    Point dest, Angle orientation, double desired_rpm, bool small_kick_allowed);

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
LLPrimitive move_shoot(Point dest, double power, bool chip);

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
LLPrimitive move_shoot(Point dest, Angle orientation, double power, bool chip);

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
LLPrimitive move_catch(double velocity, double dribbler_speed, double margin);

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
LLPrimitive move_pivot(Point centre, Angle swing, Angle orientation);

/**
 * \brief Spins around rapidly while moving.
 *
 * \param[in] dest the position to move to, as a distance forward
 * and left of the robot’s current position, relative to the
 * robot’s current orientation
 * \param[in] speed the speed to spin at, in units per second
 */
LLPrimitive move_spin(Point dest, Angle speed);
}
