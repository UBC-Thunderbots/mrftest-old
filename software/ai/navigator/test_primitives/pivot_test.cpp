#include "pivot_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
PivotTest::PivotTest(World w)
    : PrimTest(),
      centre(Point()),
      orient(Angle::zero()),
      swing(Angle::zero()),
      world(w)
{
    tests["Pivot"] = static_cast<testfun_t>(&PivotTest::test_pivot);
    build_widget();
}

void PivotTest::test_pivot(Player player)
{
    player.move_pivot(centre, swing, orient);
}

void PivotTest::build_widget()
{
    // set the range of values for the X and Y sliders
    point_x_slider.set_range(
        -world.field().length() / 2, world.field().length() / 2);
    point_y_slider.set_range(
        -world.field().width() / 2, world.field().width() / 2);
    angle_entry.set_range(
        -360, 360);  // change range? 360 degrees sounds excessive
    swing_angle_entry.set_range(
        -360, 360);  // change range? 360 degrees sounds excessive

    // make some labels
    x_label.set_label("X Coordinate");
    y_label.set_label("Y Coordinate");
    angle_label.set_label("Robot orientation (degrees)");
    swing_angle_label.set_label("Angle to pivot (degrees)");

    box.add(x_label);
    box.add(point_x_slider);  // x for dest

    box.add(y_label);
    box.add(point_y_slider);  // y for dest

    box.add(angle_label);
    box.add(angle_entry);  // orient

    box.add(swing_angle_label);
    box.add(swing_angle_entry);  // pivot angle

    // show the box
    box.show_all();

    // link signals

    point_x_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &PivotTest::on_point_x_changed));
    point_y_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &PivotTest::on_point_y_changed));
    angle_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &PivotTest::on_angle_changed));
    swing_angle_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &PivotTest::on_swing_angle_changed));
}

void PivotTest::on_point_x_changed()
{
    centre = Point(point_x_slider.get_value(), centre.y);
}

void PivotTest::on_point_y_changed()
{
    centre = Point(centre.x, point_y_slider.get_value());
}

void PivotTest::on_angle_changed()
{
    orient = Angle::of_degrees(angle_entry.get_value());
}

void PivotTest::on_swing_angle_changed()
{
    swing = Angle::of_degrees(swing_angle_entry.get_value());
}

Gtk::Widget& PivotTest::get_widget()
{
    return box;
}
}
}
}
