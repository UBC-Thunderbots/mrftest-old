#include "ai/backend/ball.h"
#include "ai/backend/backend.h"
#include "util/param.h"

using AI::BE::Ball;

namespace
{
DoubleParam BALL_DECAY_CONSTANT(
    u8"Ball Decay Constant", u8"AI/Backend", 99.0, 0.0, 100.0);
}

Ball::Ball()
    : should_highlight(false),
      pred(
          1.3e-3, 4, std::chrono::duration_cast<Predictor<double>::Timediff>(
                         std::chrono::duration<double>(BALL_DECAY_CONSTANT)))
{
}

void Ball::add_field_data(Point pos, AI::Timestamp ts)
{
    pred.add_measurement(
        pos,
        ts + std::chrono::duration_cast<AI::Timediff>(
                 std::chrono::duration<double>(LOOP_DELAY)));
    update_caches();
}

void Ball::lock_time(AI::Timestamp now)
{
    pred.lock_time(now);
    update_caches();
}

AI::Timestamp Ball::lock_time() const
{
    return pred.lock_time();
}

Point Ball::position(double delta) const
{
    return pred.value(delta).first;
}

Point Ball::velocity(double delta) const
{
    return pred.value(delta, 1).first;
}

Point Ball::position_stdev(double delta) const
{
    return pred.value(delta).second;
}

Point Ball::velocity_stdev(double delta) const
{
    return pred.value(delta, 1).second;
}

bool Ball::highlight() const
{
    return should_highlight;
}

Visualizable::Colour Ball::highlight_colour() const
{
    return Visualizable::Colour(0.0, 0.0, 0.0);
}

void Ball::update_caches()
{
    position_cached = position(0.0);
    velocity_cached = velocity(0.0);
}
