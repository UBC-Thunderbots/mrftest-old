#include "ai/navigator/test_primitives/dribble_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
DribbleTest::DribbleTest(World w)
    : PrimTest(w),
      dest(Point()),
      orient(Angle::of_degrees(45)),
      desired_rpm(0),
      world(w),
      small_kick_allowed(false)
{
    tests["Dribble"] = static_cast<testfun_t>(&DribbleTest::test_dribble);

    // NEED POINT DEST, ANGLE ORIENT, DESIRED RPM, SMALL KICK ALLOWED (CHECK
    // BOX)
    // Control Elements
    x_coord_slider = std::make_shared<SliderControlElement>(
        "X Coordinate", -world.field().length() / 2,
        world.field().length() / 2);
    y_coord_slider = std::make_shared<SliderControlElement>(
        "Y Coordinate", -world.field().width() / 2, world.field().width() / 2);
    angle_slider =
        std::make_shared<SliderControlElement>("Angle (degrees)", -180, 180);
    desired_rpm_slider =
        std::make_shared<SliderControlElement>("RPM", 0, 20000);
    small_kick_allowed_checkbutton =
        std::make_shared<CheckbuttonControlElement>("Use Small Kick");
    use_ball_coords_checkbutton =
        std::make_shared<CheckbuttonControlElement>("use ball coords");
    make_widget();
}
// makes widget
void DribbleTest::make_widget()
{
    control_elements.push_back(x_coord_slider);
    control_elements.push_back(y_coord_slider);
    control_elements.push_back(angle_slider);
    control_elements.push_back(desired_rpm_slider);
    control_elements.push_back(small_kick_allowed_checkbutton);
    control_elements.push_back(use_ball_coords_checkbutton);
    use_ball_coords_checkbutton->GetCheckbutton()->signal_clicked().connect(
        sigc::mem_fun(this, &DribbleTest::on_use_ball_coordinates_changed));
    build_widget();
}

void DribbleTest::test_dribble(Player player)
{
    player.send_prim(
        Drive::move_dribble(dest, orient, desired_rpm, small_kick_allowed));
}
// updates the x coordinates,y coordinate,RPM and small kick checkbox
void DribbleTest::update_params()
{
    dest        = Point(x_coord_slider->GetValue(), y_coord_slider->GetValue());
    orient      = Angle::of_degrees(angle_slider->GetValue());
    desired_rpm = desired_rpm_slider->GetValue();
    small_kick_allowed = small_kick_allowed_checkbutton->GetValue();
}
// small kick
void DribbleTest::on_small_kick_allowed_changed()
{
    small_kick_allowed =
        small_kick_allowed_checkbutton->GetCheckbutton()->get_active();
}

void DribbleTest::on_use_ball_coordinates_changed()
{
    if (use_ball_coords_checkbutton->GetCheckbutton()->get_active())
    {
        x_coord_slider->GetHScale()->set_sensitive(false);
        y_coord_slider->GetHScale()->set_sensitive(false);
        use_ball_coords = true;
    }
    else
    {
        x_coord_slider->GetHScale()->set_sensitive(true);
        y_coord_slider->GetHScale()->set_sensitive(true);
        dest = Point(x_coord_slider->GetValue(), y_coord_slider->GetValue());
        use_ball_coords = false;
    }
}
}
}
}