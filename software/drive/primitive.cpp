#include "drive/primitive.h"
#include "util/dprint.h"

Drive::LLPrimitive::LLPrimitive()
    : prim(static_cast<Drive::Primitive>(-1)),
      params({-1, -1, -1, -1}),
      extra(0)
{
}

Drive::LLPrimitive::LLPrimitive(
    Drive::Primitive prim, std::vector<double> p, uint8_t extra)
    : prim(prim), params(p), extra(extra)
{
    if (p.size() > PARAMS_MAX_SIZE)
        LOG_WARN(u8"Params vector too large, extra values will be ignored.");
}

Point Drive::LLPrimitive::field_point() const
{
    return Point(params[0] / 1000.0, params[1] / 1000.0);
}

Angle Drive::LLPrimitive::field_angle() const
{
    return Angle::of_radians(params[2] / 100.0);
}

Drive::LLPrimitive Drive::move_coast()
{
    return Drive::LLPrimitive(Drive::Primitive::STOP, {0.0, 0.0, 0.0, 0.0}, 0);
}

Drive::LLPrimitive Drive::move_brake()
{
    return Drive::LLPrimitive(Drive::Primitive::STOP, {0.0, 0.0, 0.0, 0.0}, 1);
}

Drive::LLPrimitive Drive::move_move(Point dest, double end_speed)
{
    return Drive::LLPrimitive(
        Drive::Primitive::MOVE,
        {dest.x * 1000.0, dest.y * 1000.0, 0.0, end_speed * 1000.0}, 0);
}

Drive::LLPrimitive Drive::move_move(
    Point dest, Angle orientation, double end_speed)
{
    return Drive::LLPrimitive(
        Drive::Primitive::MOVE,
        {dest.x * 1000.0, dest.y * 1000.0,
         orientation.angle_mod().to_radians() * 100.0, end_speed * 1000.0},
        1);
}

Drive::LLPrimitive Drive::move_dribble(
    Point dest, Angle orientation, double desired_rpm, bool small_kick_allowed)
{
    return Drive::LLPrimitive(
        Drive::Primitive::DRIBBLE,
        {dest.x * 1000.0, dest.y * 1000.0,
         orientation.angle_mod().to_radians() * 100.0, desired_rpm},
        small_kick_allowed);
}

Drive::LLPrimitive Drive::move_shoot(Point dest, double power, bool chip)
{
    return Drive::LLPrimitive(
        Drive::Primitive::SHOOT,
        {dest.x * 1000.0, dest.y * 1000.0, 0.0, power * 1000.0}, chip);
}

Drive::LLPrimitive Drive::move_shoot(
    Point dest, Angle orientation, double power, bool chip)
{
    return Drive::LLPrimitive(
        Drive::Primitive::SHOOT,
        {dest.x * 1000.0, dest.y * 1000.0,
         orientation.angle_mod().to_radians() * 100.0, power * 1000.0},
        static_cast<uint8_t>(2 | chip));
}

Drive::LLPrimitive Drive::move_catch(
    Angle angle_diff, double displacement, double speed)
{
    return Drive::LLPrimitive(
        Drive::Primitive::CATCH,
        {angle_diff.angle_mod().to_radians() * 100.0, displacement * 1000.0,
         speed * 1000.0, 0.0},
        0);
}

Drive::LLPrimitive Drive::move_pivot(
    Point centre, Angle swing, Angle orientation)
{
    return Drive::LLPrimitive(
        Drive::Primitive::PIVOT,
        {centre.x * 1000.0, centre.y * 1000.0,
         swing.angle_mod().to_radians() * 100.0,
         orientation.angle_mod().to_radians() * 100.0},
        0);
}

Drive::LLPrimitive Drive::move_spin(Point dest, Angle speed)
{
    return Drive::LLPrimitive(
        Drive::Primitive::SPIN,
        {dest.x * 1000.0, dest.y * 1000.0, speed.to_radians() * 100.0, 0.0}, 0);
}