#include "catch_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
CatchTest::CatchTest(World w)
    : PrimTest(), orient(Angle::zero()), displacement(0), speed(0), world(w)
{
    tests["Catch"] = static_cast<testfun_t>(&CatchTest::test_catch);
    build_widget();
}

void CatchTest::test_catch(Player player)
{
    player.move_catch(orient, displacement, speed);
}

void CatchTest::build_widget()
{
    // Create input range
    speed_entry.set_range(0, 20);
    angle_entry.set_range(-360, 360);
    displacement_entry.set_range(-50, 50);

    // Generate labels Names
    speed_label.set_label("Speed");
    angle_label.set_label("Angle (degrees)");
    displacement_label.set_label("Displacement");

    // add widgets to vbox
    box.add(speed_label);
    box.add(speed_entry);  // speed

    box.add(angle_label);
    box.add(angle_entry);  // orient

    box.add(displacement_label);
    box.add(displacement_entry);  // displacement

    box.show_all();

    // link signals
    speed_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &CatchTest::on_speed_changed));
    angle_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &CatchTest::on_angle_changed));
    displacement_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &CatchTest::on_displacement_changed));
}

void CatchTest::on_speed_changed()
{
    speed = speed_entry.get_value();
}

void CatchTest::on_angle_changed()
{
    orient = Angle::of_degrees(angle_entry.get_value());
}

void CatchTest::on_displacement_changed()
{
    displacement = displacement_entry.get_value();
}
}
}
}
