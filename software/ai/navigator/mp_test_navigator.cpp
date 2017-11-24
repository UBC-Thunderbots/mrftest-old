#include "mp_test_navigator.h"
// #include "test_primitives/catch_test.h"
// #include "test_primitives/dribble_test.h"
#include "test_primitives/move_test.h"
// #include "test_primitives/pivot_test.h"
// #include "test_primitives/shoot_test.h"
// #include "test_primitives/spin_test.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace AI::Nav::TestNavigator;

namespace AI
{
namespace Nav
{
namespace TestNavigator
{

MPTest::MPTest(AI::Nav::W::World world) : Navigator(world)
{
    build_gui();
}

/**
* \brief Builds the test navigator UI box
*/
void MPTest::build_gui()
{
    // combo.append(CHOOSE_PLAY_TEXT);

    // primitives["MoveSpin"]    = new SpinTest(world);
    primitives["MoveMove"] = std::shared_ptr<PrimTest>(new MoveTest(world));
    primitives[CHOOSE_PLAY_TEXT] =
        std::shared_ptr<PrimTest>(new PrimTest(world));

    // primitives["MoveShoot"]   = new ShootTest(world);
    // primitives["MoveDribble"] = new DribbleTest(world);
    // primitives["MoveCatch"]   = new CatchTest(world);
    // primitives["MovePivot"]   = new PivotTest(world);

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

/**
* \brief updates the player of the current test and calls the test function
* every tick if looping_test_fun is true.
*/
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
}
}
}

NAVIGATOR_REGISTER(MPTest)
