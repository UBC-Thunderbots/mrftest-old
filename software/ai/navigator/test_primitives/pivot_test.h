#pragma once
#include "ai/navigator/mp_test_navigator.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
class PivotTest : public PrimTest
{
   public:
    /**
    * \brief The point to pivot around
    */
    Point centre;
    /**
    * \brief Orientation of the robot
    */
    Angle orient;
    /**
    * \brief Angle to pivot
    */
    Angle swing;

    /**
    * \brief Whether to use ball coordinates as center of pivot
    */
    bool goto_ball;

    std::shared_ptr<SliderControlElement> x_coord_slider;
    std::shared_ptr<SliderControlElement> y_coord_slider;
    std::shared_ptr<SliderControlElement> orient_slider;
    std::shared_ptr<SliderControlElement> swing_slider;

    std::shared_ptr<CheckbuttonControlElement> goto_ball_checkbutton;

    PivotTest(World w);

    void make_widget();

    void update_params();

    void test_pivot(Player player);

    void on_goto_ball_coords_changed();

    void on_chip_changed();
};
}
}
}