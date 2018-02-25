#include "ai/navigator/test_primitives/spin_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
SpinTest::SpinTest(World w)
    : PrimTest(w), dest(Point()), speed(Angle::zero()), world(w)
{
    tests["Spin"] = static_cast<testfun_t>(&SpinTest::test_spin);
    x_coord_slider = std::make_shared<SliderControlElement>(
        "X Coordinate (m)", -world.field().length() / 2,
        world.field().length() / 2);
    y_coord_slider = std::make_shared<SliderControlElement>(
        "Y Coordinate (m)", -world.field().width() / 2, world.field().width() / 2);
    speed_slider = std::make_shared<SliderControlElement>("Speed (deg/s)", 0, 1260);
    make_widget();
}

// Sliders for coords and speed
void SpinTest::make_widget()
{
    control_elements.push_back(x_coord_slider);
    control_elements.push_back(y_coord_slider);
    control_elements.push_back(speed_slider);
    build_widget();
}

void SpinTest::test_spin(Player player)
{
    player.move_spin(dest, speed);
}

// Updates parameters
void SpinTest::update_params()
{
    dest  = Point(x_coord_slider->GetValue(), y_coord_slider->GetValue());
    speed = Angle::of_degrees(speed_slider->GetValue());
}

}
}
}