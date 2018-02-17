#pragma once
#include "ai/navigator/mp_test_navigator.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
class MoveTest : public PrimTest
{
   public:
    Point dest;
    Angle orient;
    double time_delta;
    World world;
    bool goto_ball;

    // Shared_ptrs to the control elements
    std::shared_ptr<SliderControlElement> x_coord_slider;
    std::shared_ptr<SliderControlElement> y_coord_slider;
    std::shared_ptr<SliderControlElement> angle_slider;
    std::shared_ptr<SliderControlElement> time_delta_slider;
    std::shared_ptr<CheckbuttonControlElement> goto_ball_checkbutton;

    // Constructor
    MoveTest(World w);

    // Function that creates adds the control widgets to the PrimTest vector
    void make_widget();

    // Callback to update params (overrides default PrimTest impl)
    void update_params() override;

    // Callback functions used to execute the MPs
    void test_move_dest(Player player);

    void test_move_ori_dest(Player player);

    void test_move_tdelta_dest(Player player);

    void test_move_ori_tdelta_dest(Player player);

    void test_move_to_ball(Player player);
    
    void test_move_replace(Player player);

    void on_goto_ball_coords_changed();
};
}
}
}