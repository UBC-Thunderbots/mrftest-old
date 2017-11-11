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

    World world;

    PivotTest(World w);
    void test_pivot(Player player);

    /**
    * \brief X and Y sliders
    */
    Gtk::HScale point_x_slider;
    Gtk::HScale point_y_slider;
    /**
    * \brief Angle field
    */
    Gtk::SpinButton angle_entry;
    /**
    * \brief Pivot angle field
    */
    Gtk::SpinButton swing_angle_entry;

    Gtk::Label x_label;
    Gtk::Label y_label;
    Gtk::Label angle_label;
    Gtk::Label swing_angle_label;

    void on_point_x_changed();
    void on_point_y_changed();
    void on_angle_changed();
    void on_swing_angle_changed();

    void build_widget();
    Gtk::Widget& get_widget() override;
};
}
}
}