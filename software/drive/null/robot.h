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
    void send_prim(Drive::LLPrimitive p) override;
    void move_slow(bool slow = true) override;
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
