#include "ai/navigator/test_primitives/shoot_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
ShootTest::ShootTest(World w)
    : PrimTest(),
      dest(Point()),
      power(0),
      chip(false),
      orient(Angle::zero()),
      world(w),
      goto_ball(false)
{
    tests["Shoot"]       = static_cast<testfun_t>(&ShootTest::test_shoot);
    tests["ShootOrient"] = static_cast<testfun_t>(&ShootTest::test_shoot_ori);
    build_widget();
}

void ShootTest::test_shoot(Player player)
{
    if (goto_ball)
        dest = world.ball().position();
    Angle orientation =
        (world.field().enemy_goal() - world.ball().position()).orientation();
    player.move_shoot(dest, orientation, power, chip);
}

void ShootTest::test_shoot_ori(Player player)
{
    if (goto_ball)
        dest = world.ball().position();
    player.move_shoot(dest, orient, power, chip);
}

void ShootTest::build_widget()
{
    point_x_slider.set_range(
        -world.field().length() / 2, world.field().length() / 2);
    point_y_slider.set_range(
        -world.field().width() / 2, world.field().width() / 2);

    angle_entry.set_range(-360, 360);
    power_entry.set_range(0, 50000);

    // point_x_slider.set_name("Field X",false);
    // point_y_slider.set_name("Field Y",false);

    to_chip.set_label("Chip");

    x_label.set_label("X Coordinate");
    y_label.set_label("Y Coordinate");
    angle_label.set_label("Angle (degrees)");
    powerLbl.set_label("Power");
    goto_ball_coords_checkbox.set_label("Use ball coordinates");

    // add widgets to vbox
    box.add(x_label);
    box.add(point_x_slider);  // x for dest

    box.add(y_label);
    box.add(point_y_slider);  // y for dest

    box.add(goto_ball_coords_checkbox);

    box.add(to_chip);  // chip

    box.add(angle_label);
    box.add(angle_entry);  // orient

    box.add(powerLbl);
    box.add(power_entry);  // power

    box.show_all();

    // link signals
    point_x_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &ShootTest::on_point_x_changed));
    point_y_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &ShootTest::on_point_y_changed));
    to_chip.signal_clicked().connect(
        sigc::mem_fun(this, &ShootTest::on_chip_changed));
    angle_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &ShootTest::on_angle_changed));
    power_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &ShootTest::on_power_changed));
    goto_ball_coords_checkbox.signal_clicked().connect(
        sigc::mem_fun(this, &ShootTest::on_goto_ball_coords_changed));
}

void ShootTest::on_point_x_changed()
{
    dest = Point(point_x_slider.get_value(), dest.y);
}
void ShootTest::on_point_y_changed()
{
    dest = Point(dest.x, point_y_slider.get_value());
}

void ShootTest::on_chip_changed()
{
    chip = to_chip.get_active();
}

void ShootTest::on_angle_changed()
{
    orient = Angle::of_degrees(angle_entry.get_value());
}

void ShootTest::on_power_changed()
{
    power = power_entry.get_value();
}

void ShootTest::on_goto_ball_coords_changed()
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