#pragma once
#include "ai/navigator/mp_test_navigator.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
class SpinTest : public PrimTest
{
   public:
    Point dest;
    /**
     * \brief sets x and y coords
     */
    Angle speed;
    /**
     * \brief defines speed as an angle
     */

    World world;

    std::shared_ptr<SliderControlElement> x_coord_slider;
    std::shared_ptr<SliderControlElement> y_coord_slider;
    std::shared_ptr<SliderControlElement> speed_slider;
    SpinTest(World w);
    void make_widget();
    /**
     * \brief creates widget ( x and y sliders/ speed slider/ labels )
     *
     */
    void update_params();
    void test_spin(Player player);
    /**
     * \brief takes in the player
     *
     */
};
}
}
}