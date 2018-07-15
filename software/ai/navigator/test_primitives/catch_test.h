#pragma once
#include "ai/navigator/mp_test_navigator.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
class CatchTest : public PrimTest
{
   public:
    /**
     * \brief Global orientation of the robot
     */
    Angle orient;

    /**
     * \brief ????
     *
     */
    double displacement;

    /**
     * \brief Speed of the robot.
     *
     */
    double speed;

    World world;

    /**
     * \brief Constructor.
     * \param[in] w World that contains players.
     */
    CatchTest(World w);

    /**
     * \brief wrapper function for movement primitive
     * \param[in] player the player to execute the primitve
     */
    void test_catch(Player player);

    // Callback to update params (overrides default PrimTest impl)
    void update_params() override;

    // GTK ControlElement shared_ptrs
    std::shared_ptr<SliderControlElement> displacement_slider;
    std::shared_ptr<SliderControlElement> angle_slider;
    std::shared_ptr<SliderControlElement> speed_slider;
};
}
}
}