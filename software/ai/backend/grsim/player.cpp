#include "ai/backend/grsim/player.h"
#include <cmath>
#include <iostream>
#include "ai/backend/ball.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include "util/dprint.h"

using AI::BE::GRSim::Player;
using std::chrono::steady_clock;

namespace
{
#warning The robot geometry here should be updated to match our own once we have a custom grSim INI file.
constexpr steady_clock::duration CHICKER_CHARGE_TIME = std::chrono::seconds(2);
constexpr double ROBOT_DISTANCE_TO_FRONT             = 0.073;
constexpr double BALL_DIAMETER                       = 0.043;
constexpr double HAS_BALL_THRESHOLD                  = 0.01;
constexpr double DRIBBLER_WIDTH                      = 0.08;
constexpr double HALF_DRIBBLER_WIDTH                 = DRIBBLER_WIDTH / 2.0;
const Angle HALF_DRIBBLER_ARC =
    Angle::atan(HALF_DRIBBLER_WIDTH / ROBOT_DISTANCE_TO_FRONT);
constexpr Angle HALF_DRIBBLER_ARC_MARGIN  = Angle::of_degrees(-2.0);
constexpr double MAX_CHIP_SPEED           = 3.603f;
constexpr double CHIP_ANGLE               = 51.483f;
const double SINE_CHIP_ANGLE              = std::sin(CHIP_ANGLE);
const double COSINE_CHIP_ANGLE            = std::cos(CHIP_ANGLE);
const double CONTROLLER_PROPORTIONAL_GAIN = 3;
const double ORI_GAIN                     = 4;
constexpr double
    RADIANS_PER_SECOND__PER__QUARTER_DEGREES_PER_FIVE_MILLISECONDS =
        200 /* 5ms / s */ * 0.25 /* degrees per quarter thereof */ * M_PI /
        180.0 /* radians per degree */;
const double MAX_SPEED = 2.0;

Point linear_controller(Point dest)
{
    if (dest.lensq() > 0.4 * 0.4)
    {
        return dest.norm() * MAX_SPEED / 2.5;
    }
    else
    {
        return dest * CONTROLLER_PROPORTIONAL_GAIN;
    }
}
}

Player::Player(unsigned int pattern, const AI::BE::Ball &ball)
    : AI::BE::Player(pattern),
      _prim(Property<Drive::Primitive>(Drive::Primitive::STOP)),
      _prim_extra(0),
      _ball(ball),
      _autokick_fired(false),
      _had_ball(false),
      _last_chick_time(steady_clock::now())
{
}

bool Player::has_ball() const
{
    Point ball_contact_point =
        _ball.position() -
        Point::of_angle(orientation(0.0)) * BALL_DIAMETER / 2.0;
    Angle contact_point_offset_angle = (ball_contact_point - position(0.0))
                                           .orientation()
                                           .angle_diff(orientation(0.0));
    if (!(contact_point_offset_angle <=
          HALF_DRIBBLER_ARC + HALF_DRIBBLER_ARC_MARGIN))
    {
        return false;
    }
    double distance_to_ball_contact_point =
        (ball_contact_point - position(0.0)).len();
    double projected_distance_to_centre_of_dribbler =
        distance_to_ball_contact_point * contact_point_offset_angle.cos();
    return projected_distance_to_centre_of_dribbler <=
           ROBOT_DISTANCE_TO_FRONT + HAS_BALL_THRESHOLD;
}
double Player::get_lps(unsigned int) const
{
    return 0.0;
}

bool Player::chicker_ready() const
{
    return steady_clock::now() - _last_chick_time >= CHICKER_CHARGE_TIME;
}

bool Player::autokick_fired() const
{
    return _autokick_fired;
}

void Player::tick(bool halt, bool stop)
{
    // should we reset these values?
    _autokick_fired   = false;
    _drive_linear     = Point(0, 0);
    _drive_angular    = Angle::zero();
    _drive_spinner    = false;
    _drive_kickspeedx = 0;
    _drive_kickspeedz = 0;

    bool lhas_ball = has_ball();

    if (halt)
    {
        return;
    }

    Angle local_move_ori = _move_ori - this->orientation();
    Point local_dest     = _move_dest - this->position();
    local_dest           = local_dest.rotate(-this->orientation());
    switch (_prim.get())
    {
        case Drive::Primitive::STOP:
        {
            if (_prim_extra)
            {
                _drive_linear  = Point(0, 0);
                _drive_angular = Angle::zero();
            }
            else
            {
#warning does grsim give velocity data?
                _drive_linear  = velocity() * -CONTROLLER_PROPORTIONAL_GAIN;
                _drive_angular = avelocity() * -ORI_GAIN;
            }

            break;
        }
        case Drive::Primitive::MOVE:
        {
#warning time delta is ignored
            _drive_linear = linear_controller(local_dest);

            if (_prim_extra)
            {
                _drive_angular = local_move_ori * ORI_GAIN;
            }
            else
            {
                _drive_angular = Angle::zero();
            }

            break;
        }
        case Drive::Primitive::DRIBBLE:
        {
            _drive_linear  = linear_controller(local_dest);
            _drive_angular = local_move_ori * ORI_GAIN;
#warning small kick is ignored

            _drive_spinner = true;

            break;
        }
        case Drive::Primitive::SHOOT:
        {
            _drive_linear  = linear_controller(local_dest);
            _drive_angular = local_move_ori * ORI_GAIN;

            if (lhas_ball && local_move_ori.to_degrees() < 5)
            {  // within 5 deg
                if (_prim_extra & 1)
                {  // chip
                    _drive_kickspeedx =
                        static_cast<float>(COSINE_CHIP_ANGLE * _shoot_power);
                    _drive_kickspeedz =
                        static_cast<float>(SINE_CHIP_ANGLE * _shoot_power);
                }
                else
                {
                    _drive_kickspeedx = static_cast<float>(_shoot_power);
                }
            }

            if (_had_ball && !lhas_ball)
            {
                _autokick_fired = true;
            }

            break;
        }
        case Drive::Primitive::CATCH:
        {
            Point linear_transformed = Point(
                _catch_speed,
                _catch_displacement * CONTROLLER_PROPORTIONAL_GAIN);
            _drive_linear  = linear_transformed.rotate(-local_move_ori);
            _drive_angular = local_move_ori * ORI_GAIN;
            _drive_spinner = true;

            break;
        }
        case Drive::Primitive::PIVOT:
        {
#warning this is totally wrong
            if (_pivot_swing.abs().to_degrees() < 10)
            {
                _drive_angular = local_move_ori * ORI_GAIN;
            }
            else
            {
                _drive_linear = _move_dest
                                    .rotate(
                                        _pivot_swing.to_radians() > 0
                                            ? Angle::three_quarter()
                                            : Angle::quarter())
                                    .norm() *
                                MAX_SPEED;
            }

            break;
        }
        case Drive::Primitive::SPIN:
        {
#warning not implemented
            break;
        }
        case Drive::Primitive::DIRECT_WHEELS:
        {
#warning not implemented
            break;
        }
        case Drive::Primitive::DIRECT_VELOCITY:
        {
#warning not implemented
            break;
        }
    }

    _had_ball = lhas_ball;

    if (stop)
    {
        _drive_spinner = false;
    }
}

void Player::replace_ball(
    grSim_BallReplacement &ball_replacement, double x, double y, double vx,
    double vy)
{
    ball_replacement.set_x(x);
    ball_replacement.set_y(y);
    ball_replacement.set_vx(vx);
    ball_replacement.set_vy(vy);
}

void Player::encode_orders(grSim_Robot_Command &packet)
{
    packet.set_id(pattern());
    packet.set_wheelsspeed(false);

    packet.set_veltangent(static_cast<float>(_drive_linear.x));
    packet.set_velnormal(static_cast<float>(_drive_linear.y));
    packet.set_velangular(static_cast<float>(_drive_angular.to_radians()));
    packet.set_kickspeedx(static_cast<float>(_drive_kickspeedx));
    packet.set_kickspeedz(static_cast<float>(_drive_kickspeedz));
    packet.set_spinner(_drive_spinner);
}
