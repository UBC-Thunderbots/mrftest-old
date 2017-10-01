#ifndef DRIVE_NULL_ROBOT_H
#define DRIVE_NULL_ROBOT_H

#include "drive/robot.h"

namespace Drive
{
namespace Null
{
class Dongle;

class Robot final : public Drive::Robot
{
   public:
    Robot(Drive::Null::Dongle &dongle);
    Drive::Dongle &dongle() override;
    const Drive::Dongle &dongle() const override;
    void set_charger_state(ChargerState state) override;
    void move_slow(bool slow = true) override;
    void move_coast() override;
    void move_brake() override;
    void move_move(Point dest) override;
    void move_move(Point dest, Angle orientation) override;
    void move_move(Point dest, double end_speed) override;
    void move_move(Point dest, Angle orientation, double end_speed) override;
    void move_dribble(
        Point dest, Angle orientation, double desired_rpm,
        bool small_kick_allowed) override;
    void move_shoot(Point dest, double power, bool chip) override;
    void move_shoot(
        Point dest, Angle orientation, double power, bool chip) override;
    void move_catch(
        Angle angle_diff, double displacement, double speed) override;
    void move_pivot(Point centre, Angle swing, Angle orientation) override;
    void move_spin(Point dest, Angle speed) override;
    void direct_wheels(const int (&wheels)[4]) override;
    void direct_velocity(Point vel, Angle avel) override;
    void direct_dribbler(unsigned int rpm) override;
    void direct_chicker(double power, bool chip) override;
    void direct_chicker_auto(double power, bool chip) override;

   private:
    Drive::Null::Dongle &the_dongle;
};
}
}

#endif
