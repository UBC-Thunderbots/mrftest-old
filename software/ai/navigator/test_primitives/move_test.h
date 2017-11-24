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

    std::shared_ptr<SliderControlElement> x_coord_slider;
    std::shared_ptr<SliderControlElement> y_coord_slider;
    std::shared_ptr<SliderControlElement> angle_slider;
    std::shared_ptr<SliderControlElement> time_delta_slider;
    std::shared_ptr<CheckbuttonControlElement> goto_ball_checkbutton;
    MoveTest(World w);
    void make_widget();
    void update_params();
    void test_move_dest(Player player);

    void test_move_ori_dest(Player player);

    void test_move_tdelta_dest(Player player);

    void test_move_ori_tdelta_dest(Player player);

    void test_move_to_ball(Player player);

    void on_goto_ball_coords_changed();
};
}
}
}