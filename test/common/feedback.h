#ifndef TEST_COMMON_FEEDBACK_H
#define TEST_COMMON_FEEDBACK_H

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/levelbar.h>
#include <gtkmm/table.h>
#include "drive/dongle.h"
#include "drive/robot.h"

/**
 * \brief A panel that lets the user view various pieces of information about a
 * robot.
 */
class TesterFeedbackPanel final : public Gtk::Table
{
   public:
    /**
     * \brief Constructs a new TesterFeedbackPanel.
     *
     * \param[in] dongle the radio dongle over which to communicate.
     *
     * \param[in] robot the robot whose information should be displayed.
     */
    explicit TesterFeedbackPanel(Drive::Dongle &dongle, Drive::Robot &robot);

   private:
    Drive::Dongle &dongle;
    Drive::Robot &robot;
    Gtk::Label battery_voltage_label, capacitor_voltage_label,
        dribbler_temperature_label, board_temperature_label, break_beam_label,
        lqi_label, rssi_label, fw_build_id_label, fpga_build_id_label,
        lps_val_label;
    Gtk::LevelBar battery_voltage_bar, capacitor_voltage_bar,
        dribbler_temperature_bar, dribbler_speed_bar, board_temperature_bar,
        break_beam_bar, lqi_bar, rssi_bar;
    Gtk::Label battery_voltage_value_label, capacitor_voltage_value_label,
        dribbler_temperature_value_label, dribbler_speed_value_label,
        board_temperature_value_label, break_beam_value_label, lqi_value_label,
        rssi_value_label;
    Gtk::Entry fw_build_id, fpga_build_id, lps_val;
    Gtk::HBox cb_hbox1, cb_hbox2;
    Gtk::CheckButton alive, estop, ball_in_beam, capacitor_charged;

    void on_battery_voltage_changed();
    void on_capacitor_voltage_changed();
    void on_dribbler_temperature_changed();
    void on_dribbler_speed_changed();
    void on_board_temperature_changed();
    void on_break_beam_reading_changed();
    void on_lqi_changed();
    void on_rssi_changed();
    void on_fw_build_id_changed();
    void on_fpga_build_id_changed();
    void on_lps_val_changed();
    void on_alive_changed();
    void on_estop_changed();
    void on_ball_in_beam_changed();
    void on_capacitor_charged_changed();
};

#endif
