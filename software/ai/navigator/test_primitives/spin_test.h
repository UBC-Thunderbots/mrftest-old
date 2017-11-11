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
    /**
     * \brief defines a point for the robot to move to
     */
    Point dest;

    /**
     * \brief defines angular velocity of robot
     */
    Angle speed;

    World world;

    // GTK elements for the sliders defining dest
    Gtk::HScale point_x_slider;
    Gtk::HScale point_y_slider;

    // GTK element defining the angular velocity
    Gtk::SpinButton speed_entry;

    Gtk::CheckButton goto_ball_coords_checkbox;

    Gtk::Label x_label;
    Gtk::Label y_label;
    Gtk::Label speed_label;

    bool goto_ball;

    /**
     * \brief Constructor.
     * \param[in] w World that contains players.
     */
    SpinTest(World w);

    /**
     * \brief wrapper function for movement primitive
     * \param[in] player the player to execute the primitve
     */
    void test_spin(Player player);

    /**
     * \brief builds the controls for the test
     *
     */
    void build_widget();

    /**
     * \brief Adjusts the x-component of dest when the slider is moved
     *
     */
    void on_point_x_changed();

    /**
     * \brief Adjusts the y-component of dest when the slider is moved
     *
     */
    void on_point_y_changed();

    void on_speed_changed();

    /**
     * \brief Adjusts the y-component of dest when the slider is moved
     *
     */
    void on_goto_ball_coords_changed();
};
}
}
}