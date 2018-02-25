#pragma once
#include "ai/navigator/mp_test_navigator.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
class ShootTest : public PrimTest
{
   public:
    Point dest;
    double power;
    bool chip;
    Angle orient;

    World world;
    bool goto_ball;

    std::shared_ptr<SliderControlElement> x_coord_slider;
    std::shared_ptr<SliderControlElement> y_coord_slider;
    std::shared_ptr<SliderControlElement> angle_slider;
    std::shared_ptr<SliderControlElement> power_slider;
    std::shared_ptr<CheckbuttonControlElement> chip_checkbutton;
    std::shared_ptr<CheckbuttonControlElement> goto_ball_checkbutton;

    ShootTest(World w);

    // builds widget
    void make_widget();

    // updates the parameters of coordinates,angle, power and checkbottons
    void update_params();

    void test_shoot(Player player);

    void test_shoot_ori(Player player);

    // when checkbox clicked, robot goes to ball
    void on_goto_ball_coords_changed();

    // chipper activation
    void on_chip_changed();
};
}
}
}