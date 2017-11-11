#pragma once
#include "ai/navigator/mp_test_navigator.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
class ShootTest : public PrimTest
{
   public:
    Point dest;
    double power;
    bool chip;
    Angle orient;

    World world;
    bool has_shot = false;

    // Gtk::VBox box;
    Gtk::HScale point_x_slider;
    Gtk::HScale point_y_slider;
    Gtk::SpinButton power_entry;
    Gtk::SpinButton angle_entry;
    Gtk::CheckButton to_chip;

    Gtk::CheckButton goto_ball_coords_checkbox;

    Gtk::Label x_label;
    Gtk::Label y_label;
    Gtk::Label powerLbl;
    Gtk::Label angle_label;

    bool goto_ball;

    ShootTest(World w);

    void test_shoot(Player player);

    void test_shoot_ori(Player player);

    void build_widget();

    void on_point_x_changed();

    void on_point_y_changed();

    void on_chip_changed();

    void on_angle_changed();

    void on_power_changed();

    void on_goto_ball_coords_changed();
};
}
}
}