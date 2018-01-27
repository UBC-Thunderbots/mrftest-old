#include "ai/navigator/test_primitives/move_test.h"

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
MoveTest::MoveTest(World w)
    : PrimTest(),
      dest(Point(0, 0)),
      orient(Angle::of_degrees(45)),
      time_delta(0),
      world(w),
      goto_ball(false)
{
    tests["Move"]       = static_cast<testfun_t>(&MoveTest::test_move_dest);
    tests["MoveReplace"] = static_cast<testfun_t>(&MoveTest::test_move_replace);
    tests["MoveOrient"] = static_cast<testfun_t>(&MoveTest::test_move_ori_dest);
    tests["MoveTimeDelta"] =
        static_cast<testfun_t>(&MoveTest::test_move_tdelta_dest);
    tests["MoveOrientTimeDelta"] =
        static_cast<testfun_t>(&MoveTest::test_move_ori_tdelta_dest);
    tests["MoveToBall"] = static_cast<testfun_t>(&MoveTest::test_move_to_ball);
    build_widget();
}

void MoveTest::test_move_dest(Player player)
{
    LOG_INFO(u8"Called");
    if (goto_ball)
        dest = world.ball().position();
    player.move_move(dest);
}

void MoveTest::test_move_replace(Player player)
{
    if (world.friendly_team().size() > 0)
    {
        FriendlyTeam friendly_team = world.friendly_team();
        int friendly_size          = static_cast<int>(friendly_team.size());
        for (int i = 0; i < friendly_size; i++)
        {
            Player player = friendly_team[i];
            player.replace(0.3 + (0.4 * i), 0, 0.0, i, true);
        }
    }
    if (world.enemy_team().size() > 0)
    {
        EnemyTeam enemy_team = world.enemy_team();
        int enemy_size       = static_cast<int>(enemy_team.size());
        for (int i = 0; i < enemy_size; i++)
        {
            Robot enemy = enemy_team[i];
            enemy.replace(-0.3 * (i + 1), -1.5, 0.0, i, false);
        }
    }
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

void MoveTest::build_widget()
{
    point_x_slider.set_range(
        -world.field().length() / 2, world.field().length() / 2);
    point_y_slider.set_range(
        -world.field().width() / 2, world.field().width() / 2);
    angle_entry.set_range(
        -360, 360);  // change range? 360 degrees sounds excessive
    time_entry.set_range(-100, 100);  // dunno

    x_label.set_label("X Coordinate");
    y_label.set_label("Y Coordinate");
    angle_label.set_label("Angle (degrees)");
    time_label.set_label("Time Delta");
    goto_ball_coords_checkbox.set_label("Use ball coordinates");

    // add widgets to vbox
    box.add(x_label);
    box.add(point_x_slider);  // x for dest

    box.add(y_label);
    box.add(point_y_slider);  // y for dest

    // add the checkbox to use ball coordinates
    box.add(goto_ball_coords_checkbox);

    box.add(angle_label);
    box.add(angle_entry);  // orient

    box.add(time_label);
    box.add(time_entry);  // power

    box.show_all();

    // link signals
    point_x_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &MoveTest::on_point_x_changed));
    point_y_slider.signal_value_changed().connect(
        sigc::mem_fun(this, &MoveTest::on_point_y_changed));
    angle_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &MoveTest::on_angle_changed));
    time_entry.signal_value_changed().connect(
        sigc::mem_fun(this, &MoveTest::on_time_changed));
    goto_ball_coords_checkbox.signal_clicked().connect(
        sigc::mem_fun(this, &MoveTest::on_goto_ball_coords_changed));
}

void MoveTest::on_point_x_changed()
{
    dest = Point(point_x_slider.get_value(), dest.y);
}
void MoveTest::on_point_y_changed()
{
    dest = Point(dest.x, point_y_slider.get_value());
}

void MoveTest::on_angle_changed()
{
    orient = Angle::of_degrees(angle_entry.get_value());
}
void MoveTest::on_time_changed()
{
    time_delta = time_entry.get_value();
}

void MoveTest::on_goto_ball_coords_changed()
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