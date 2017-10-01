#ifndef TEST_COMMON_KICKER_H
#define TEST_COMMON_KICKER_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/scale.h>
#include <gtkmm/table.h>
#include <gtkmm/togglebutton.h>
#include "drive/robot.h"
#include "util/property.h"

/**
 * \brief A panel that lets the user manually control the kicking subsystem.
 */
class KickerPanel final : public Gtk::Table
{
   public:
    /**
     * \brief Constructs a new KickerPanel.
     *
     * \param[in] robot the robot to control.
     */
    explicit KickerPanel(Drive::Robot &robot);

    /**
     * \brief Shuts down the charger.
     */
    void scram();

    /**
     * \brief Fires the solenoids.
     */
    void fire();

   private:
    Drive::Robot &robot;
    Gtk::HBox charge_box;
    Gtk::RadioButtonGroup charge_group;
    Gtk::RadioButton discharge_button, float_button, charge_button;
    Gtk::HBox solenoid_box;
    Gtk::RadioButtonGroup solenoid_group;
    Gtk::RadioButton kicker_button, chipper_button;
    Gtk::Label pulse_width_label;
    Gtk::HScale pulse_width;
    // New slide bar
    Gtk::Label chip_distance_label;
    Gtk::HScale chip_distance;
    Gtk::HBox fire_hbox;
    Gtk::Button kick;
    Gtk::ToggleButton autokick;
    Gtk::Label autokick_count_label;
    Gtk::Label autokick_count_value_label;
    unsigned int autokick_count;

    void on_charge_changed();
    void on_pulse_width_changed();
    void on_chip_distance_changed();
    void on_kick();
    void on_autokick_changed();
    void on_autokick_fired();
    void update_sensitive();
};

#endif
