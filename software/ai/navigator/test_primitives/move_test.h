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

    // Gtk::VBox box;
    Gtk::HScale point_x_slider;
    Gtk::HScale point_y_slider;
    Gtk::SpinButton time_entry;
    Gtk::SpinButton angle_entry;

    Gtk::CheckButton goto_ball_coords_checkbox;

    Gtk::Label x_label;
    Gtk::Label y_label;
    Gtk::Label time_label;
    Gtk::Label angle_label;

    bool goto_ball;

    MoveTest(World w);

    void test_move_dest(Player player);

    void test_move_ori_dest(Player player);

    void test_move_tdelta_dest(Player player);

    void test_move_ori_tdelta_dest(Player player);

    void test_move_to_ball(Player player);

    void build_widget();

    void on_point_x_changed();

    void on_point_y_changed();

    void on_angle_changed();

    void on_time_changed();

    void on_goto_ball_coords_changed();
};
}
}
}