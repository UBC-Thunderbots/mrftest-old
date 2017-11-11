#pragma once
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

namespace AI
{
namespace Nav
{
namespace TestNavigator
{
/**
 * \brief Default text shown when no test is selected.
 */
static const std::string CHOOSE_TEST_TEXT = "<CHOOSE_TEST>";

// Forward declaration
class PrimTest;

/**
* \brief Function pointer definition for a test function
*/
typedef void (PrimTest::*testfun_t)(Player);

/**
* \brief Class for a primitive test. All tests are subclasses of this class.
*/
class PrimTest
{
   public:
    /**
    * \brief A map of the test name to the test function pointer
    */
    std::map<std::string, testfun_t> tests;
    /**
    * \brief The current test function being executed
    */
    testfun_t current_test_fun;

    /**
    * \brief The widget box that contains UI elements for setting test params
    */
    Gtk::VBox box;

    /**
    * \brief The button that starts the test
    */
    Gtk::Button activate;
    /**
    * \brief The player that will do the movement primitive
    */
    Player player;

    /**
    * \brief True if the test function loops
    */
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

    /**
    * \brief Calls the test function.
    */
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
};
}
}
}