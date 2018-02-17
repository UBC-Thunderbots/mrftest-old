#include "ai/navigator/test_primitives/shoot_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
ShootTest::ShootTest(World w)
    : PrimTest(world),
      dest(Point(0, 0)),
      orient(Angle::of_degrees(45)),
      power(100),
      chip(false),
      world(w),
      goto_ball(false)
{
    tests["Shoot"] = static_cast<testfun_t>(&ShootTest::test_shoot_ori);

    x_coord_slider = std::make_shared<SliderControlElement>(
        "X Coordinate", -world.field().length() / 2,
        world.field().length() / 2);
    y_coord_slider = std::make_shared<SliderControlElement>(
        "Y Coordinate", -world.field().width() / 2, world.field().width() / 2);
    angle_slider =
        std::make_shared<SliderControlElement>("Angle (degrees)", -180, 180);
    power_slider     = std::make_shared<SliderControlElement>("Power", 0, 100);
    chip_checkbutton = std::make_shared<CheckbuttonControlElement>("Chip");
    goto_ball_checkbutton =
        std::make_shared<CheckbuttonControlElement>("Use ball coordinates");
    make_widget();
}


void ShootTest::test_shoot(Player player)
{
    if (goto_ball)
    {
        dest = world.ball().position();
    }
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
//function builds test Nav
//builds widget
void ShootTest::make_widget()
{
    control_elements.push_back(x_coord_slider);
    control_elements.push_back(y_coord_slider);
    control_elements.push_back(goto_ball_checkbutton);
    goto_ball_checkbutton->GetCheckbutton()->signal_clicked().connect(
        sigc::mem_fun(this, &ShootTest::on_goto_ball_coords_changed));
    control_elements.push_back(angle_slider);
    control_elements.push_back(power_slider);
    control_elements.push_back(chip_checkbutton);
    chip_checkbutton->GetCheckbutton()->signal_clicked().connect(
        sigc::mem_fun(this, &ShootTest::on_chip_changed));
    build_widget();
}

//updates parameters for coords,angle and power
void ShootTest::update_params()
{
    dest   = Point(x_coord_slider->GetValue(), y_coord_slider->GetValue());
    orient = Angle::of_degrees(angle_slider->GetValue());
    power  = power_slider->GetValue();
}

void ShootTest::on_goto_ball_coords_changed()
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
        dest = Point(x_coord_slider->GetValue(), y_coord_slider->GetValue());
        goto_ball = false;
    }
}
    //enables chipper when checkbutton is selected
void ShootTest::on_chip_changed()
{
    chip = chip_checkbutton->GetCheckbutton()->get_active();
}
}
}
}