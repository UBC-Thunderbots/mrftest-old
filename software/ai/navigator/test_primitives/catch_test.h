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

    // GTK control elements
    Gtk::SpinButton displacement_entry;
    Gtk::SpinButton angle_entry;
    Gtk::SpinButton speed_entry;

    // Labels for GTK elements
    Gtk::Label speed_label;
    Gtk::Label displacement_label;
    Gtk::Label angle_label;

    // Callback functions to update parameters when changed in UI
    void on_speed_changed();
    void on_angle_changed();
    void on_displacement_changed();

    void build_widget();
};
}
}
}