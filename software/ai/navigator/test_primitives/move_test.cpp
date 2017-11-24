#include "ai/navigator/test_primitives/move_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
MoveTest::MoveTest(World w)
    : PrimTest(world),
      dest(Point(0, 0)),
      orient(Angle::of_degrees(45)),
      time_delta(0),
      world(w),
      goto_ball(false)
{
    tests["Move"]       = static_cast<testfun_t>(&MoveTest::test_move_dest);
    tests["MoveOrient"] = static_cast<testfun_t>(&MoveTest::test_move_ori_dest);
    tests["MoveTimeDelta"] =
        static_cast<testfun_t>(&MoveTest::test_move_tdelta_dest);
    tests["MoveOrientTimeDelta"] =
        static_cast<testfun_t>(&MoveTest::test_move_ori_tdelta_dest);
    tests["MoveToBall"] = static_cast<testfun_t>(&MoveTest::test_move_to_ball);
    double t            = world.field().length();
    x_coord_slider =
        std::shared_ptr<SliderControlElement>(new SliderControlElement(
            "X Coordinate", -world.field().length() / 2,
            world.field().length() / 2));
    y_coord_slider =
        std::shared_ptr<SliderControlElement>(new SliderControlElement(
            "Y Coordinate", -world.field().width() / 2,
            world.field().width() / 2));
    angle_slider = std::shared_ptr<SliderControlElement>(
        new SliderControlElement("Angle (degrees)", -180, 180));
    time_delta_slider = std::shared_ptr<SliderControlElement>(
        new SliderControlElement("Time Delta", 0, 10));
    goto_ball_checkbutton = std::shared_ptr<CheckbuttonControlElement>(
        new CheckbuttonControlElement("Use ball coordinates"));
    make_widget();
}

void MoveTest::make_widget()
{
    control_elements.push_back(x_coord_slider);
    control_elements.push_back(y_coord_slider);
    control_elements.push_back(goto_ball_checkbutton);
    goto_ball_checkbutton->GetCheckbutton()->signal_clicked().connect(
        sigc::mem_fun(this, &MoveTest::on_goto_ball_coords_changed));

    control_elements.push_back(angle_slider);
    control_elements.push_back(time_delta_slider);
    build_widget();
}

void MoveTest::test_move_dest(Player player)
{
    if (goto_ball)
        dest = world.ball().position();
    player.move_move(dest);
}

void MoveTest::test_move_ori_dest(Player player)
{
    if (goto_ball)
        dest = world.ball().position();
    player.move_move(dest, orient);
}

void MoveTest::test_move_tdelta_dest(Player player)
{
    if (goto_ball)
        dest = world.ball().position();
    player.move_move(dest, time_delta);
}

void MoveTest::test_move_ori_tdelta_dest(Player player)
{
    if (goto_ball)
        dest = world.ball().position();
    player.move_move(dest, orient, time_delta);
}

void MoveTest::test_move_to_ball(Player player)
{
    player.move_move(
        world.ball().position(),
        (world.ball().position() - player.position()).orientation());
}

void MoveTest::update_params()
{
    dest       = Point(x_coord_slider->GetValue(), y_coord_slider->GetValue());
    orient     = Angle::of_degrees(angle_slider->GetValue());
    time_delta = time_delta_slider->GetValue();
}

void MoveTest::on_goto_ball_coords_changed()
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
}
}
}