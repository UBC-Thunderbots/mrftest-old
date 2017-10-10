#include <glibmm/ustring.h>
#include <gtkmm.h>
#include <functional>
#include <iostream>
#include "ai/hl/stp/param.h"
#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/main_loop.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;

namespace
{
static const std::string CHOOSE_TEST_TEXT = "<CHOOSE_TEST>";
Angle get_relative_angle(Point robotPos, Angle robotOri, Point dest)
{
    Point pos_diff          = dest - robotPos;
    Angle robot_local_angle = pos_diff.orientation() - robotOri;
    return robot_local_angle;
}

Point get_relative_velocity(Point robotPos, Angle robotOri, Point dest)
{
    Point pos_diff         = dest - robotPos;
    Point robot_local_dest = pos_diff.rotate(-robotOri).norm(0.2);
    return robot_local_dest;
}

class PrimTest;

typedef void (PrimTest::*testfun_t)(Player);

class PrimTest
{
   public:
    std::map<std::string, testfun_t> tests;
    testfun_t current_test_fun;
    Gtk::VBox box;
    Gtk::Button activate;
    Player player;
    bool looping_test_fun = false;

    PrimTest()
    {
        current_test_fun        = &PrimTest::do_nothing;
        tests[CHOOSE_TEST_TEXT] = &PrimTest::do_nothing;
        activate.set_label("Run");
        box.pack_end(activate);
        activate.signal_clicked().connect(
            sigc::mem_fun(this, &PrimTest::call_test_fun));
    }

    virtual Gtk::Widget &get_widget()
    {
        return box;
    }

    void call_test_fun()
    {
        if (current_test_fun)
        {
            (this->*current_test_fun)(player);
        }
    }

    void do_nothing(Player)
    {
    }
};
/*
        class CatchTest : public PrimTest {
                public:
                        Point dest;
                        double power;
                        bool chip;
                        Angle orient;

                        World world;
                        bool has_shot = false;

                        //Gtk::VBox box;
                        Gtk::HScale point_x_slider;
                        Gtk::HScale point_y_slider;
                        Gtk::SpinButton power_entry;
                        Gtk::SpinButton angle_entry;
                        Gtk::CheckButton to_chip;

                        Gtk::Label x_label;
                        Gtk::Label y_label;
                        Gtk::Label powerLbl;
                        Gtk::Label angle_label;

                        CatchTest(World w): PrimTest(), dest(Point()), power(0),
   chip(false), orient(Angle::zero()), world(w) {
                                tests["Catch"] =
   static_cast<testfun_t>(&CatchTest::test_shoot);
                                tests["CatchOrient"] =
   static_cast<testfun_t>(&CatchTest::test_shoot_ori);
                                build_widget();
                        }

                        void test_shoot(Player player) {
                                player.move_shoot(dest, power, chip);
                        }

                        void test_shoot_ori(Player player) {
                                player.move_shoot(dest,orient,power,chip);

                        }

                        void build_widget() {
                                point_x_slider.set_range(-world.field().length()/2,world.field().length()/2);
                                point_y_slider.set_range(-world.field().width()/2,world.field().width()/2);

                                angle_entry.set_range(-360,360);
                                power_entry.set_range(0,50000);

                                //point_x_slider.set_name("Field X",false);
                                //point_y_slider.set_name("Field Y",false);

                                to_chip.set_label("Chip");

                                x_label.set_label("X Coordinate");
                                y_label.set_label("Y Coordinate");
                                angle_label.set_label("Angle (degrees)");
                                powerLbl.set_label("Power");

                                //add widgets to vbox
                                box.add(x_label);
                                box.add(point_x_slider); //x for dest

                                box.add(y_label);
                                box.add(point_y_slider); //y for dest

                                box.add(to_chip); //chip

                                box.add(angle_label);
                                box.add(angle_entry); //orient

                                box.add(powerLbl);
                                box.add(power_entry); //power

                                box.show_all();

                                //link signals
                                point_x_slider.signal_value_changed().connect(sigc::mem_fun(this,&CatchTest::on_point_x_changed));
                                point_y_slider.signal_value_changed().connect(sigc::mem_fun(this,&CatchTest::on_point_y_changed));
                                to_chip.signal_clicked().connect(sigc::mem_fun(this,&CatchTest::on_chip_changed));
                                angle_entry.signal_value_changed().connect(sigc::mem_fun(this,&CatchTest::on_angle_changed));
                                power_entry.signal_value_changed().connect(sigc::mem_fun(this,&CatchTest::on_power_changed));
                        }

                        void on_point_x_changed() {
                                dest = Point(point_x_slider.get_value(),dest.y);

                        }
                        void on_point_y_changed() {
                                dest = Point(dest.x,point_y_slider.get_value());
                        }

                        void on_chip_changed() {
                                chip = to_chip.get_active();
                        }

                        void on_angle_changed() {
                                orient =
   Angle::of_degrees(angle_entry.get_value());
                        }
                        void on_power_changed() {
                                power = power_entry.get_value();
                        }

                        Gtk::Widget &get_widget() override {
                                return box;
                        }
        };
*/
class ShootTest : public PrimTest
{
   public:
    Point dest;
    double power;
    bool chip;
    Angle orient;

    World world;
    bool has_shot = false;

    // Gtk::VBox box;
    Gtk::HScale point_x_slider;
    Gtk::HScale point_y_slider;
    Gtk::SpinButton power_entry;
    Gtk::SpinButton angle_entry;
    Gtk::CheckButton to_chip;

    Gtk::Label x_label;
    Gtk::Label y_label;
    Gtk::Label powerLbl;
    Gtk::Label angle_label;

    ShootTest(World w)
        : PrimTest(),
          dest(Point()),
          power(0),
          chip(false),
          orient(Angle::zero()),
          world(w)
    {
        tests["Shoot"] = static_cast<testfun_t>(&ShootTest::test_shoot);
        tests["ShootOrient"] =
            static_cast<testfun_t>(&ShootTest::test_shoot_ori);
        build_widget();
    }

    void test_shoot(Player player)
    {
        player.move_shoot(dest, power, chip);
    }

    void test_shoot_ori(Player player)
    {
        player.move_shoot(dest, orient, power, chip);
    }

    void build_widget()
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

        // add widgets to vbox
        box.add(x_label);
        box.add(point_x_slider);  // x for dest

        box.add(y_label);
        box.add(point_y_slider);  // y for dest

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
    }

    void on_point_x_changed()
    {
        dest = Point(point_x_slider.get_value(), dest.y);
    }
    void on_point_y_changed()
    {
        dest = Point(dest.x, point_y_slider.get_value());
    }

    void on_chip_changed()
    {
        chip = to_chip.get_active();
    }

    void on_angle_changed()
    {
        orient = Angle::of_degrees(angle_entry.get_value());
    }
    void on_power_changed()
    {
        power = power_entry.get_value();
    }

    Gtk::Widget &get_widget() override
    {
        return box;
    }
};

class MoveTest : public PrimTest
{
   public:
    Point dest;
    Angle orient;
    double time_delta;
    World world;

    // Gtk::VBox box;
    Gtk::HScale point_x_slider;
    Gtk::HScale point_y_slider;
    Gtk::SpinButton time_entry;
    Gtk::SpinButton angle_entry;

    Gtk::Label x_label;
    Gtk::Label y_label;
    Gtk::Label time_label;
    Gtk::Label angle_label;

    MoveTest(World w)
        : PrimTest(),
          dest(Point(0, 0)),
          orient(Angle::of_degrees(45)),
          time_delta(0),
          world(w)
    {
        tests["Move"] = static_cast<testfun_t>(&MoveTest::test_move_dest);
        tests["MoveOrient"] =
            static_cast<testfun_t>(&MoveTest::test_move_ori_dest);
        tests["MoveTimeDelta"] =
            static_cast<testfun_t>(&MoveTest::test_move_tdelta_dest);
        tests["MoveOrientTimeDelta"] =
            static_cast<testfun_t>(&MoveTest::test_move_ori_tdelta_dest);
        tests["MoveToBall"] =
            static_cast<testfun_t>(&MoveTest::test_move_to_ball);
        build_widget();
    }

    void test_move_dest(Player player)
    {
        LOG_INFO(u8"Called");
        player.move_move(dest);
    }

    void test_move_ori_dest(Player player)
    {
        player.move_move(dest, orient);
    }

    void test_move_tdelta_dest(Player player)
    {
        player.move_move(dest, time_delta);
    }

    void test_move_ori_tdelta_dest(Player player)
    {
        player.move_move(dest, orient, time_delta);
    }

    void test_move_to_ball(Player player)
    {
        looping_test_fun       = true;
        Point robot_local_dest = get_relative_velocity(
            player.position(), player.orientation(), world.ball().position());
        Angle robot_local_angle = get_relative_angle(
            player.position(), player.orientation(), world.ball().position());
        player.move_move(robot_local_dest, robot_local_angle);
    }

    void build_widget()
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

        // add widgets to vbox
        box.add(x_label);
        box.add(point_x_slider);  // x for dest

        box.add(y_label);
        box.add(point_y_slider);  // y for dest

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
    }

    void on_point_x_changed()
    {
        dest = Point(point_x_slider.get_value(), dest.y);
    }
    void on_point_y_changed()
    {
        dest = Point(dest.x, point_y_slider.get_value());
    }

    void on_angle_changed()
    {
        orient = Angle::of_degrees(angle_entry.get_value());
    }
    void on_time_changed()
    {
        time_delta = time_entry.get_value();
    }

    Gtk::Widget &get_widget() override
    {
        return box;
    }
};

class DribbleTest : public PrimTest
{
   public:
    Point dest;
    Angle orient;
    bool small_kick_allowed;
    double rpm;

    DribbleTest()
        : dest(Point()),
          orient(Angle::of_degrees(45)),
          small_kick_allowed(true),
          rpm(0)
    {
        tests["Dribble"] = static_cast<testfun_t>(&DribbleTest::test_dribble);
    }

    DribbleTest(Point d, Angle a, double r, bool s)
        : dest(d), orient(a), small_kick_allowed(s), rpm(r)
    {
        tests["Dribble"] = static_cast<testfun_t>(&DribbleTest::test_dribble);
    }

    void test_dribble(Player player)
    {
        player.move_dribble(dest, orient, rpm, small_kick_allowed);
    }
};

class PivotTest : public PrimTest
{
   public:
    Point centre;
    Angle swing;
    Angle orient;

    PivotTest()
        : centre(Point()),
          swing(Angle::of_degrees(45)),
          orient(Angle::of_degrees(45))
    {
        tests["Pivot"] = static_cast<testfun_t>(&PivotTest::test_pivot);
    }

    PivotTest(Point c, Angle s, Angle a) : centre(c), swing(s), orient(a)
    {
        tests["Pivot"] = static_cast<testfun_t>(&PivotTest::test_pivot);
    }

    void test_pivot(Player player)
    {
        player.move_pivot(centre, swing, orient);
    }
};

class SpinTest : public PrimTest
{
   public:
    Point dest;
    Angle speed;

    SpinTest() : dest(Point()), speed(Angle::zero())
    {
        tests["Spin"] = static_cast<testfun_t>(&SpinTest::test_spin);
    }

    SpinTest(Point d, Angle s) : dest(d), speed(s)
    {
        tests["Spin"] = static_cast<testfun_t>(&SpinTest::test_spin);
    }

    void test_spin(Player player)
    {
        player.move_spin(dest, speed);
    }
};

class BezierTest : public PrimTest
{
   public:
    // quadratic
    Point p0;
    Point p1;
    Point p2;
    double t;

    BezierTest() : p0(Point()), p1(Point()), p2(Point()), t(0)
    {
    }

    BezierTest(Point p0, Point p1, Point p2, double t)
        : p0(p0), p1(p1), p2(p2), t(t)
    {
    }
};
}

namespace AI
{
namespace Nav
{
namespace MPTest
{
/*
 * A navigator that tests the functions of movement primitives.
 */
class MPTest final : public Navigator
{
   public:
    void tick() override;
    void on_combo_changed();
    void on_test_combo_changed();

    explicit MPTest(World world);
    NavigatorFactory &factory() const override;

    Gtk::Widget *ui_controls() override;

   private:
    const Glib::ustring CHOOSE_PLAY_TEXT = u8"<Choose Play>";
    void build_gui();

    // The current active movement primitive test set
    PrimTest *current_test = new PrimTest();

    // A map of displayed names to primitive test sets
    std::map<std::string, PrimTest *> primitives;

    Gtk::VBox vbox;
    Gtk::ComboBoxText combo;
    Gtk::ComboBoxText test_combo;
    // Gtk::Widget *primTestControls;
};
}
}
}

using namespace AI::Nav::MPTest;

MPTest::MPTest(AI::Nav::W::World world) : Navigator(world)
{
    build_gui();
}

void MPTest::build_gui()
{
    combo.append(CHOOSE_PLAY_TEXT);

    primitives["MoveSpin"]    = new SpinTest();
    primitives["MoveMove"]    = new MoveTest(world);
    primitives["MoveShoot"]   = new ShootTest(world);
    primitives["MoveDribble"] = new DribbleTest();
    primitives["MovePivot"]   = new PivotTest();

    for (auto const &i : primitives)
    {
        combo.append(Glib::ustring(i.first));
    }

    combo.set_active_text(CHOOSE_PLAY_TEXT);
    vbox.add(combo);

    vbox.add(test_combo);

    combo.signal_changed().connect(
        sigc::mem_fun(this, &MPTest::on_combo_changed));
    test_combo.signal_changed().connect(
        sigc::mem_fun(this, &MPTest::on_test_combo_changed));
    vbox.add(current_test->get_widget());
}

void MPTest::tick()
{
    if (world.friendly_team().size() > 0)
    {
        current_test->player = world.friendly_team()[0];
        if (current_test->looping_test_fun)
        {
            current_test->call_test_fun();
        }
    }
}

Gtk::Widget *MPTest::ui_controls()
{
    return &vbox;
}

void MPTest::on_combo_changed()
{
    vbox.remove(current_test->get_widget());

    current_test = primitives[combo.get_active_text()];

    test_combo.remove_all();
    for (auto const &i : current_test->tests)
    {
        test_combo.append(Glib::ustring(i.first));
    }
    test_combo.set_active_text(CHOOSE_TEST_TEXT);

    vbox.add(current_test->get_widget());
    current_test->looping_test_fun = false;
}

void MPTest::on_test_combo_changed()
{
    if (current_test->tests.find(test_combo.get_active_text()) ==
        current_test->tests.end())
    {
        current_test->current_test_fun = current_test->tests[CHOOSE_TEST_TEXT];
    }
    else
    {
        current_test->current_test_fun =
            current_test->tests[test_combo.get_active_text()];
    }
    current_test->looping_test_fun = false;
}

NAVIGATOR_REGISTER(MPTest)
