#include "ai/navigator/test_primitives/pivot_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
PivotTest::PivotTest(World w)
    : PrimTest(w),
      centre(Point(0, 0)),
      orient(Angle::of_degrees(45)),
      swing(Angle::of_degrees(45))
{
    tests["Pivot"] = static_cast<testfun_t>(&PivotTest::test_pivot);

    x_coord_slider = std::make_shared<SliderControlElement>(
        "X Coordinate (m)", -world.field().length() / 2,
        world.field().length() / 2);
    y_coord_slider = std::make_shared<SliderControlElement>(
        "Y Coordinate (m)", -world.field().width() / 2,
        world.field().width() / 2);
    orient_slider = std::make_shared<SliderControlElement>(
        "Orientation of robot (degrees)", -180, 180);
    swing_slider = std::make_shared<SliderControlElement>(
        "Angle to pivot (degrees)", -180, 180);
    goto_ball_checkbutton =
        std::make_shared<CheckbuttonControlElement>("Use ball coordinates");
    make_widget();
}

void PivotTest::test_pivot(Player player)
{
    player.move_pivot(centre, swing, orient);
}

void PivotTest::make_widget()
{
    control_elements.push_back(x_coord_slider);
    control_elements.push_back(y_coord_slider);
    control_elements.push_back(orient_slider);
    control_elements.push_back(swing_slider);
    control_elements.push_back(goto_ball_checkbutton);
    goto_ball_checkbutton->GetCheckbutton()->signal_clicked().connect(
        sigc::mem_fun(this, &PivotTest::on_goto_ball_coords_changed));
    build_widget();
}

void PivotTest::update_params()
{
    centre = Point(x_coord_slider->GetValue(), y_coord_slider->GetValue());
    orient = Angle::of_degrees(orient_slider->GetValue());
    swing  = Angle::of_degrees(swing_slider->GetValue());
}

void PivotTest::on_goto_ball_coords_changed()
{
    if (goto_ball_checkbutton->GetCheckbutton()->get_active())
    {
        x_coord_slider->GetHScale()->set_sensitive(false);
        y_coord_slider->GetHScale()->set_sensitive(false);
        goto_ball = true;
    }
    else
    {
        x_coord_slider->GetHScale()->set_sensitive(true);
        y_coord_slider->GetHScale()->set_sensitive(true);
        centre = Point(x_coord_slider->GetValue(), y_coord_slider->GetValue());
        goto_ball = false;
    }
}
}
}
}