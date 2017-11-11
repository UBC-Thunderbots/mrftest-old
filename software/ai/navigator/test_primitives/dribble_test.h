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
    Angle orientation;
    double desired_rpm;
    bool small_kick_allowed;

    World world;

    Gtk::HScale point_x_slider;
    Gtk::HScale point_y_slider;
    Gtk::SpinButton angle_entry;
    Gtk::SpinButton rpm_entry;
    Gtk::CheckButton small_kick_allowed_checkbox;
    Gtk::CheckButton goto_ball_coords_checkbox;

    Gtk::Label x_label;
    Gtk::Label y_label;
    Gtk::Label angle_label;
    Gtk::Label rpm_label;

    bool goto_ball;

    DribbleTest(World _world);

    void test_dribble(Player player);

    void test_shoot(Player player);

    void test_shoot_ori(Player player);

    void build_widget();

    void on_point_x_changed();

    void on_point_y_changed();

    void on_rpm_changed();

    void on_angle_changed();

    void on_small_kick_allowed_changed();

    void on_goto_ball_coords_changed();
};
}
}
}