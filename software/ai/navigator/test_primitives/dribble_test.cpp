#include "ai/navigator/test_primitives/dribble_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
DribbleTest::DribbleTest(World _world)
    : dest(Point()),
      orientation(Angle::zero()),
      desired_rpm(0),
      small_kick_allowed(false),
      world(_world),
      goto_ball(false)
{
    tests["Dribble"] = static_cast<testfun_t>(&DribbleTest::test_dribble);
    build_widget();
}

void DribbleTest::test_dribble(Player player)
{
    if (goto_ball)
        dest = world.ball().position();
    player.move_dribble(dest, orientation, desired_rpm, small_kick_allowed);
}

void DribbleTest::build_widget()
{
    point_x_slider.set_range(
        -world.field().length() / 2, world.field().length() / 2);
    point_y_slider.set_range(
        -world.field().width() / 2, world.field().width() / 2);
    angle_entry.set_range(
        -360, 360);  // change range? 360 degrees sounds excessive
    rpm_entry.set_range(0, 10000);

    x_label.set_label("X Coordinate");
    y_label.set_label("Y Coordinate");
    angle_label.set_label("Angle (degrees)");
    rpm_label.set_label("Desired dribbler RPM");

    goto_ball_coords_checkbox.set_label("Use ball coordinates");

    small_kick_allowed_checkbox.set_label("Small kick allowed");

    // add widgets to vbox
    box.add(x_label);
    box.add(point_x_slider);  // x for dest

    box.add(y_label);
    box.add(point_y_slider);  // y for dest

    box.add(goto_ball_coords_checkbox);

    box.add(angle_label);
    box.add(angle_entry);  // orient

    box.add(rpm_label);
    box.add(rpm_entry);  // power

    box.add(small_kick_allowed_checkbox);

    box.show_all();

    // link signals
    point_x_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &DribbleTest::on_point_x_changed));
    point_y_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &DribbleTest::on_point_y_changed));
    angle_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &DribbleTest::on_angle_changed));
    rpm_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &DribbleTest::on_rpm_changed));
    small_kick_allowed_checkbox.signal_clicked().connect(
        sigc::mem_fun(this, &DribbleTest::on_small_kick_allowed_changed));
}

void DribbleTest::on_point_x_changed()
{
    dest = Point(point_x_slider.get_value(), dest.y);
}

void DribbleTest::on_point_y_changed()
{
    dest = Point(dest.x, point_y_slider.get_value());
}

void DribbleTest::on_small_kick_allowed_changed()
{
    small_kick_allowed = small_kick_allowed_checkbox.get_active();
}

void DribbleTest::on_angle_changed()
{
    orientation = Angle::of_degrees(angle_entry.get_value());
}

void DribbleTest::on_rpm_changed()
{
    desired_rpm = rpm_entry.get_value();
}

void DribbleTest::on_goto_ball_coords_changed()
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