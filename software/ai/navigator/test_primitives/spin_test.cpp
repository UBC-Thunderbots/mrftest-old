#include "ai/navigator/test_primitives/spin_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
SpinTest::SpinTest(World w)
    : dest(Point()), speed(Angle::zero()), world(w), goto_ball(false)
{
    tests["Spin"] = static_cast<testfun_t>(&SpinTest::test_spin);
    build_widget();
}

void SpinTest::test_spin(Player player)
{
    LOG_INFO(u8"Called");
    if (goto_ball)
        dest = world.ball().position();
    player.move_spin(dest, speed);
}

void SpinTest::build_widget()
{
    point_x_slider.set_range(
        -world.field().length() / 2, world.field().length() / 2);
    point_y_slider.set_range(
        -world.field().width() / 2, world.field().width() / 2);
    speed_entry.set_range(
        -360, 360);  // change range? 360 degrees sounds excessive

    x_label.set_label("X Coordinate");
    y_label.set_label("Y Coordinate");
    speed_label.set_label("Speed");
    goto_ball_coords_checkbox.set_label("Use ball coordinates");

    // add widgets to vbox
    box.add(x_label);
    box.add(point_x_slider);  // x for dest

    box.add(y_label);
    box.add(point_y_slider);  // y for dest

    // add the checkbox to use ball coordinates
    box.add(goto_ball_coords_checkbox);

    box.add(speed_label);
    box.add(speed_entry);

    box.show_all();

    // link signals
    point_x_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &SpinTest::on_point_x_changed));
    point_y_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &SpinTest::on_point_y_changed));
    speed_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &SpinTest::on_speed_changed));
    goto_ball_coords_checkbox.signal_clicked().connect(
        sigc::mem_fun(this, &SpinTest::on_goto_ball_coords_changed));
}

void SpinTest::on_point_x_changed()
{
    dest = Point(point_x_slider.get_value(), dest.y);
}
void SpinTest::on_point_y_changed()
{
    dest = Point(dest.x, point_y_slider.get_value());
}

void SpinTest::on_speed_changed()
{
    speed = Angle::of_degrees(speed_entry.get_value());
}

void SpinTest::on_goto_ball_coords_changed()
{
    if (goto_ball_coords_checkbox.get_active())
    {
        point_x_slider.set_sensitive(false);
        point_y_slider.set_sensitive(false);
        goto_ball = true;
    }
    else
    {
        point_x_slider.set_sensitive(true);
        point_y_slider.set_sensitive(true);
        dest = Point(point_x_slider.get_value(), point_y_slider.get_value());
        goto_ball = false;
    }
}
}
}
}