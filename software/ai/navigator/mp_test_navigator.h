#pragma once
#include <glibmm/ustring.h>
#include <gtkmm.h>
#include <functional>
#include <iostream>
#include <memory>
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
static const std::string CHOOSE_TEST_TEXT = "<CHOOSE TEST>";

// Base class for a control element to be rendered in the test nav window.
class ControlElement
{
   public:
    std::shared_ptr<Gtk::Widget> control_widget;
    Gtk::Label label;

   protected:
    // Callback when change occurs.
    void callback_func();
};

// A ControlElement that contains a slider (Gtk::HScale)
class SliderControlElement : public ControlElement
{
   public:
    /**
      * Constructs a new SliderControlElement.
      *
      * \param[in] lbl The displayed name above the slider.
      *
      * \param[in] range_min The minimum range of the slider.
      *
      * \param[in] range_max The maximum range of the slider.
      *
      */
    SliderControlElement(std::string lbl, double range_min, double range_max)
    {
        control_widget = std::shared_ptr<Gtk::HScale>(new Gtk::HScale());
        std::shared_ptr<Gtk::HScale> hScale = this->GetHScale();
        label.set_label(lbl);

        hScale->set_range(range_min, range_max);
        hScale->signal_value_changed().connect(
            sigc::mem_fun(this, &SliderControlElement::callback_func));
    }

    /**
     * Returns the currently selected value of the slider.
     *
     * \return the value of the slider
     */
    double GetValue()
    {
        return slider_value;
    }

    std::shared_ptr<Gtk::HScale> GetHScale()
    {
        return std::dynamic_pointer_cast<Gtk::HScale>(control_widget);
    }

   private:
    double slider_value;

    void callback_func()
    {
        slider_value = this->GetHScale()->get_value();
    }
};

// This class wraps a Gtk CheckButton
class CheckbuttonControlElement : public ControlElement
{
   public:
    CheckbuttonControlElement(std::string lbl)
    {
        control_widget =
            std::shared_ptr<Gtk::CheckButton>(new Gtk::CheckButton());
        std::shared_ptr<Gtk::CheckButton> check_button = this->GetCheckbutton();
        check_button->set_label(lbl);

        check_button->signal_clicked().connect(
            sigc::mem_fun(this, &CheckbuttonControlElement::callback_func));
    }

    bool GetValue()
    {
        return checkbutton_value;
    }

    std::shared_ptr<Gtk::CheckButton> GetCheckbutton()
    {
        return std::dynamic_pointer_cast<Gtk::CheckButton>(
            this->control_widget);
    }

   private:
    bool checkbutton_value;
    void callback_func()
    {
        checkbutton_value = this->GetCheckbutton()->get_active();
    }
};

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
    * \brief Vector of control elements to be rendered
    */
    std::vector<std::shared_ptr<ControlElement>> control_elements;
    /**
    * \brief The button that starts the test
    */
    Gtk::Button activate;

    World world;
    /**
    * \brief The player that will do the movement primitive
    */
    Player player;

    /**
    * \brief True if the test function loops
    */
    bool looping_test_fun = false;

    // Callback used to update values from GUI
    virtual void update_params()
    {
    }

    PrimTest(World w) : world(w)
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

    void build_widget()
    {
        for (auto &i : control_elements)
        {
            box.add(i->label);
            box.add(*(i->control_widget));
        }
        box.show_all();
    }
    /**
    * \brief Calls the test function.
    */
    void call_test_fun()
    {
        update_params();
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
    explicit MPTest(AI::Nav::W::World world);
    void tick() override;

    void on_combo_changed();
    void on_test_combo_changed();

    NavigatorFactory &factory() const override;

    Gtk::Widget *ui_controls() override;

   private:
    const Glib::ustring CHOOSE_PLAY_TEXT = u8"<Choose Play>";
    void build_gui();

    // The current active movement primitive test set
    std::shared_ptr<PrimTest> current_test = std::make_shared<PrimTest>(world);

    // A map of displayed names to primitive test sets
    std::map<std::string, std::shared_ptr<PrimTest>> primitives;

    Gtk::VBox vbox;
    Gtk::ComboBoxText combo;
    Gtk::ComboBoxText test_combo;
};
}
}
}