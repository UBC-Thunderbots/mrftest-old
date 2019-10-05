#include "test/common/feedback.h"
#include <iomanip>
#include "util/algorithm.h"
#include "util/string.h"

TesterFeedbackPanel::TesterFeedbackPanel(
    Drive::Dongle &dongle, Drive::Robot &robot)
    : Gtk::Table(12, 3),
      dongle(dongle),
      robot(robot),
      battery_voltage_label(u8"Battery:"),
      capacitor_voltage_label(u8"Capacitor:"),
      dribbler_temperature_label(u8"Dribbler:"),
      board_temperature_label(u8"Board:"),
      break_beam_label(u8"Break Beam:"),
      lqi_label(u8"LQI:"),
      rssi_label(u8"RSSI:"),
      fw_build_id_label(u8"FW:"),
      fpga_build_id_label(u8"FPGA:"),
      lps_val_label(u8"LPS value:"),
      alive(u8"Alive"),
      estop(u8"EStop Run"),
      ball_in_beam(u8"Ball in Beam"),
      capacitor_charged(u8"Capacitor Charged")
{
    battery_voltage_bar.set_max_value(18.0);
    capacitor_voltage_bar.set_max_value(250.0);
    dribbler_temperature_bar.set_max_value(125.0);
    dribbler_speed_bar.set_max_value(50000.0);
    board_temperature_bar.set_max_value(125.0);
    break_beam_bar.set_max_value(robot.break_beam_scale);

    battery_voltage_bar.set_mode(Gtk::LEVEL_BAR_MODE_CONTINUOUS);
    capacitor_voltage_bar.set_mode(Gtk::LEVEL_BAR_MODE_CONTINUOUS);
    dribbler_temperature_bar.set_mode(Gtk::LEVEL_BAR_MODE_CONTINUOUS);
    dribbler_speed_bar.set_mode(Gtk::LEVEL_BAR_MODE_CONTINUOUS);
    board_temperature_bar.set_mode(Gtk::LEVEL_BAR_MODE_CONTINUOUS);
    break_beam_bar.set_mode(Gtk::LEVEL_BAR_MODE_CONTINUOUS);
    lqi_bar.set_mode(Gtk::LEVEL_BAR_MODE_CONTINUOUS);
    rssi_bar.set_mode(Gtk::LEVEL_BAR_MODE_CONTINUOUS);

    attach(
        battery_voltage_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        battery_voltage_bar, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        battery_voltage_value_label, 2, 3, 0, 1, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        capacitor_voltage_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        capacitor_voltage_bar, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        capacitor_voltage_value_label, 2, 3, 1, 2, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        dribbler_temperature_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        dribbler_temperature_bar, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        dribbler_temperature_value_label, 2, 3, 2, 3, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        dribbler_speed_bar, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        dribbler_speed_value_label, 2, 3, 3, 4, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        board_temperature_label, 0, 1, 4, 5, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        board_temperature_bar, 1, 2, 4, 5, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        board_temperature_value_label, 2, 3, 4, 5, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        break_beam_label, 0, 1, 5, 6, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        break_beam_bar, 1, 2, 5, 6, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        break_beam_value_label, 2, 3, 5, 6, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        lqi_label, 0, 1, 6, 7, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        lqi_bar, 1, 2, 6, 7, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
    attach(
        lqi_value_label, 2, 3, 6, 7, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        rssi_label, 0, 1, 7, 8, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        rssi_bar, 1, 2, 7, 8, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
    attach(
        rssi_value_label, 2, 3, 7, 8, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        fw_build_id_label, 0, 1, 8, 9, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        fw_build_id, 1, 3, 8, 9, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        fpga_build_id_label, 0, 1, 9, 10, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        fpga_build_id, 1, 3, 9, 10, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        lps_val_label, 0, 1, 10, 11, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    attach(
        lps_val, 1, 3, 10, 11, Gtk::EXPAND | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);

    alive.set_sensitive(false);
    estop.set_sensitive(false);
    ball_in_beam.set_sensitive(false);
    capacitor_charged.set_sensitive(false);
    fw_build_id.set_editable(false);
    fpga_build_id.set_editable(false);
    lps_val.set_editable(false);
    cb_hbox1.pack_start(alive, Gtk::PACK_EXPAND_WIDGET);
    cb_hbox1.pack_start(estop, Gtk::PACK_EXPAND_WIDGET);
    attach(
        cb_hbox1, 0, 2, 11, 12, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);
    cb_hbox2.pack_start(ball_in_beam, Gtk::PACK_EXPAND_WIDGET);
    cb_hbox2.pack_start(capacitor_charged, Gtk::PACK_EXPAND_WIDGET);
    attach(
        cb_hbox2, 0, 2, 12, 13, Gtk::SHRINK | Gtk::FILL,
        Gtk::SHRINK | Gtk::FILL);

    robot.battery_voltage.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_battery_voltage_changed));
    robot.capacitor_voltage.signal_changed().connect(sigc::mem_fun(
        this, &TesterFeedbackPanel::on_capacitor_voltage_changed));
    robot.dribbler_temperature.signal_changed().connect(sigc::mem_fun(
        this, &TesterFeedbackPanel::on_dribbler_temperature_changed));
    robot.dribbler_speed.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_dribbler_speed_changed));
    robot.board_temperature.signal_changed().connect(sigc::mem_fun(
        this, &TesterFeedbackPanel::on_board_temperature_changed));
    robot.break_beam_reading.signal_changed().connect(sigc::mem_fun(
        this, &TesterFeedbackPanel::on_break_beam_reading_changed));
    robot.link_quality.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_lqi_changed));
    robot.received_signal_strength.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_rssi_changed));
    robot.build_ids_valid.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_fw_build_id_changed));
    robot.build_ids_valid.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_fpga_build_id_changed));
    robot.fw_build_id.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_fw_build_id_changed));
    robot.lps_values[0].signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_lps_val_changed));
    robot.fpga_build_id.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_fpga_build_id_changed));
    robot.alive.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_alive_changed));
    dongle.estop_state.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_estop_changed));
    robot.ball_in_beam.signal_changed().connect(
        sigc::mem_fun(this, &TesterFeedbackPanel::on_ball_in_beam_changed));
    robot.capacitor_charged.signal_changed().connect(sigc::mem_fun(
        this, &TesterFeedbackPanel::on_capacitor_charged_changed));

    on_battery_voltage_changed();
    on_capacitor_voltage_changed();
    on_dribbler_temperature_changed();
    on_dribbler_speed_changed();
    on_board_temperature_changed();
    on_break_beam_reading_changed();
    on_lqi_changed();
    on_rssi_changed();
    on_fw_build_id_changed();
    on_fpga_build_id_changed();
    on_lps_val_changed();
    on_alive_changed();
    on_estop_changed();
    on_ball_in_beam_changed();
    on_capacitor_charged_changed();
}

void TesterFeedbackPanel::on_battery_voltage_changed()
{
    if (robot.alive)
    {
        battery_voltage_bar.set_value(
            clamp(robot.battery_voltage.get(), 0.0, 18.0));
        battery_voltage_value_label.set_text(Glib::ustring::compose(
            u8"%1 V",
            Glib::ustring::format(
                std::fixed, std::setprecision(2), robot.battery_voltage)));
    }
    else
    {
        battery_voltage_bar.set_value(0.0);
        battery_voltage_value_label.set_text(u8"N/A");
    }
}

void TesterFeedbackPanel::on_capacitor_voltage_changed()
{
    if (robot.alive)
    {
        capacitor_voltage_bar.set_value(
            clamp(robot.capacitor_voltage.get(), 0.0, 250.0));
        capacitor_voltage_value_label.set_text(Glib::ustring::compose(
            u8"%1 V",
            Glib::ustring::format(
                std::fixed, std::setprecision(0), robot.capacitor_voltage)));
    }
    else
    {
        capacitor_voltage_bar.set_value(0);
        capacitor_voltage_value_label.set_text(u8"N/A");
    }
}

void TesterFeedbackPanel::on_dribbler_temperature_changed()
{
    if (robot.alive && 0 < robot.dribbler_temperature &&
        robot.dribbler_temperature < 200)
    {
        dribbler_temperature_bar.set_value(
            clamp(robot.dribbler_temperature.get(), 0.0, 125.0));
        dribbler_temperature_value_label.set_text(Glib::ustring::compose(
            u8"%1°C",
            Glib::ustring::format(
                std::fixed, std::setprecision(1), robot.dribbler_temperature)));
    }
    else
    {
        dribbler_temperature_bar.set_value(0);
        dribbler_temperature_value_label.set_text(u8"N/A");
    }
}

void TesterFeedbackPanel::on_dribbler_speed_changed()
{
    if (robot.alive)
    {
        dribbler_speed_bar.set_value(
            clamp(robot.dribbler_speed.get(), 0, 50000));
        dribbler_speed_value_label.set_text(Glib::ustring::compose(
            u8"%1 rpm", Glib::ustring::format(robot.dribbler_speed)));
    }
    else
    {
        dribbler_speed_bar.set_value(0);
        dribbler_speed_value_label.set_text(u8"N/A");
    }
}

void TesterFeedbackPanel::on_board_temperature_changed()
{
    if (robot.alive && 0 < robot.board_temperature &&
        robot.board_temperature < 200)
    {
        board_temperature_bar.set_value(
            clamp(robot.board_temperature.get(), 0.0, 125.0));
        board_temperature_value_label.set_text(Glib::ustring::compose(
            u8"%1°C",
            Glib::ustring::format(
                std::fixed, std::setprecision(1), robot.board_temperature)));
    }
    else
    {
        board_temperature_bar.set_value(0);
        board_temperature_value_label.set_text(u8"N/A");
    }
}

void TesterFeedbackPanel::on_break_beam_reading_changed()
{
    if (robot.alive)
    {
        break_beam_bar.set_value(
            clamp(robot.break_beam_reading.get(), 0.0, robot.break_beam_scale));
        break_beam_value_label.set_text(
            Glib::ustring::format(robot.break_beam_reading));
    }
    else
    {
        break_beam_bar.set_value(0);
        break_beam_value_label.set_text(u8"N/A");
    }
}

void TesterFeedbackPanel::on_lqi_changed()
{
    lqi_bar.set_value(robot.link_quality);
    lqi_value_label.set_text(Glib::ustring::format(robot.link_quality));
}

void TesterFeedbackPanel::on_rssi_changed()
{
    rssi_bar.set_value(clamp(
        (robot.received_signal_strength.get() + 90.0) / (90.0 - 35.0), 0.0,
        1.0));
    rssi_value_label.set_text(
        Glib::ustring::compose(u8"%1 dB", robot.received_signal_strength));
}

void TesterFeedbackPanel::on_fw_build_id_changed()
{
    if (robot.build_ids_valid)
    {
        fw_build_id.set_text(
            Glib::ustring::compose(u8"0x%1", tohex(robot.fw_build_id, 8)));
    }
    else
    {
        fw_build_id.set_text(u8"No data");
    }
}

void TesterFeedbackPanel::on_fpga_build_id_changed()
{
    if (robot.build_ids_valid)
    {
        fpga_build_id.set_text(
            Glib::ustring::compose(u8"0x%1", tohex(robot.fpga_build_id, 8)));
    }
    else
    {
        fpga_build_id.set_text(u8"No data");
    }
}

void TesterFeedbackPanel::on_lps_val_changed()
{
    if (robot.build_ids_valid)
    {
        lps_val.set_text(Glib::ustring::compose(
            u8"(%1,%2) %3", robot.lps_values[0], robot.lps_values[1],
            robot.lps_values[2]));
    }
    else
    {
        lps_val.set_text(u8"No data");
    }
}

void TesterFeedbackPanel::on_alive_changed()
{
    alive.set_active(robot.alive);
    on_battery_voltage_changed();
    on_capacitor_voltage_changed();
    on_dribbler_temperature_changed();
    on_dribbler_speed_changed();
    on_board_temperature_changed();
    on_break_beam_reading_changed();
    on_fw_build_id_changed();
    on_fpga_build_id_changed();
}

void TesterFeedbackPanel::on_estop_changed()
{
    estop.set_active(dongle.estop_state == Drive::Dongle::EStopState::RUN);
}

void TesterFeedbackPanel::on_ball_in_beam_changed()
{
    ball_in_beam.set_active(robot.alive && robot.ball_in_beam);
}

void TesterFeedbackPanel::on_capacitor_charged_changed()
{
    capacitor_charged.set_active(robot.alive && robot.capacitor_charged);
}
