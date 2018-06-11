#include "catch_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
CatchTest::CatchTest(World w)
    : PrimTest(w), orient(Angle::zero()), displacement(0), speed(0), world(w)
{
    tests["Catch"] = static_cast<testfun_t>(&CatchTest::test_catch);

    // Create ControlElements
    displacement_slider =
        std::make_shared<SliderControlElement>("Displacement", -50, 50);
    speed_slider = std::make_shared<SliderControlElement>("Speed (m/s)", 0, 20);
    angle_slider =
        std::make_shared<SliderControlElement>("Angle (degrees)", -180, 180);

    // Add ControlElements to vector
    control_elements.push_back(displacement_slider);
    control_elements.push_back(speed_slider);
    control_elements.push_back(angle_slider);

    // Important to call this function after adding all ControlElements to the
    // control_elements vector
    build_widget();
}

void CatchTest::test_catch(Player player)
{
    player.send_prim(Drive::move_catch(orient, displacement, speed));
}

// Callback function to update test function params
void CatchTest::update_params()
{
    orient       = Angle::of_degrees(angle_slider->GetValue());
    displacement = displacement_slider->GetValue();
    speed        = speed_slider->GetValue();
}
}
}
}
