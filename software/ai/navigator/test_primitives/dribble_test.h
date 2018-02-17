#pragma once
#include "ai/navigator/mp_test_navigator.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
class DribbleTest : public PrimTest
{
   public:
    Point dest;
    Angle orient;
    double desired_rpm;
    World world;
    bool small_kick_allowed;
    bool use_ball_coords;

    // Shared_ptrs to the control elements
    std::shared_ptr<SliderControlElement> x_coord_slider;
    std::shared_ptr<SliderControlElement> y_coord_slider;
    std::shared_ptr<SliderControlElement> angle_slider;
    std::shared_ptr<SliderControlElement> desired_rpm_slider;
    std::shared_ptr<CheckbuttonControlElement> small_kick_allowed_checkbutton;
    std::shared_ptr<CheckbuttonControlElement> use_ball_coords_checkbutton;

    // Constructor
    DribbleTest(World w);

    // Function that creates adds the control widgets to the PrimTest vector
    void make_widget();

    // Callback to update params (overrides default PrimTest impl)
    void update_params() override;

    // Callback functions used to execute the MPs
    void test_dribble(Player player);

    void on_small_kick_allowed_changed();

    void on_use_ball_coordinates_changed();

};
}
}
}