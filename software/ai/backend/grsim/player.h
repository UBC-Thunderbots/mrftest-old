#ifndef AI_BACKEND_GRSIM_PLAYER_H
#define AI_BACKEND_GRSIM_PLAYER_H

#include "ai/backend/player.h"
#include "proto/grSim_Commands.pb.h"
#include "proto/grSim_Replacement.pb.h"
#include "util/box_ptr.h"

namespace AI
{
namespace BE
{
class Ball;
namespace GRSim
{
class Player final : public AI::BE::Player
{
   public:
    typedef BoxPtr<Player> Ptr;

    explicit Player(unsigned int pattern, const AI::BE::Ball &ball);
    bool has_ball() const override;
    double get_lps(unsigned int index) const override;
    bool chicker_ready() const override;
    bool autokick_fired() const override;
    const Property<Drive::Primitive> &primitive() const override;
    void send_prim(Drive::LLPrimitive p) override;

    void tick(bool halt, bool stop);
    void encode_orders(grSim_Robot_Command &packet);

    double replace_ball_x;
    double replace_ball_y;
    double replace_ball_vx;
    double replace_ball_vy;

    void replace_ball(
        grSim_BallReplacement &ball_replacement, double x, double y, double vx,
        double vy);

   private:
    Point _drive_linear;
    Angle _drive_angular;
    double _drive_kickspeedx;
    double _drive_kickspeedz;
    bool _drive_spinner;

    Point _move_dest;
    Angle _move_ori;
    Angle _pivot_swing;
    double _move_end_speed;
    double _shoot_power;
    double _catch_displacement;
    double _catch_speed;

    Property<Drive::Primitive> _prim;
    int _prim_extra;

    const AI::BE::Ball &_ball;
    bool _autokick_fired;
    bool _had_ball;
    std::chrono::steady_clock::time_point _last_chick_time;
};
}
}
}

inline const Property<Drive::Primitive> &AI::BE::GRSim::Player::primitive()
    const
{
    return _prim;
}

inline void AI::BE::GRSim::Player::send_prim(Drive::LLPrimitive p)
{
    _prim       = p.prim;
    _prim_extra = p.extra;
    switch (_prim)
    {
        case Drive::Primitive::MOVE:
            _move_dest      = p.field_point();
            _move_ori       = p.field_angle();
            _move_end_speed = p.params[3] / 1000.0;
            break;

        case Drive::Primitive::DRIBBLE:
            _move_dest = p.field_point();
            _move_ori  = p.field_angle();
            break;

        case Drive::Primitive::SHOOT:
            _move_dest   = p.field_point();
            _move_ori    = p.field_angle();
            _shoot_power = p.params[3] / 1000.0;
            break;

        case Drive::Primitive::CATCH:
            _catch_displacement = p.params[1] / 1000.0;
            _catch_speed        = p.params[2] / 1000.0;
            _move_ori           = Angle::of_radians(p.params[0] / 100.0);
            break;

        case Drive::Primitive::PIVOT:
            _pivot_swing = p.field_angle();
            _move_dest   = p.field_point();
            _move_ori    = Angle::of_radians(p.params[3] / 100.0);
            break;

        case Drive::Primitive::SPIN:
            _move_dest = p.field_point();
            _move_ori  = p.field_angle();
            break;

        case Drive::Primitive::STOP:
            break;
    }
}

#endif
