#include "test/common/drive.h"
#include <glibmm/main.h>
#include <sigc++/bind_return.h>
#include <sigc++/functors/mem_fun.h>
#include "util/algorithm.h"

namespace
{
void on_execute_coast(Drive::Robot &bot, const double[4], bool)
{
    bot.move_coast();
}

void on_execute_brake(Drive::Robot &bot, const double[4], bool)
{
    bot.move_brake();
}

void on_update_permotor(Drive::Robot &bot, const double sliders[4], bool)
{
    int wheels[4];
    for (unsigned int i = 0; i < 4; ++i)
    {
        wheels[i] = clamp_symmetric(static_cast<int>(sliders[i]), 255);
    }
    bot.direct_wheels(wheels);
}

void on_update_matrix(Drive::Robot &bot, const double sliders[4], bool)
{
    bot.direct_velocity(
        {sliders[0], sliders[1]}, Angle::of_degrees(sliders[2]));
}

void get_low_sensitivity_scale_factors_matrix(double scale[4])
{
    scale[0] = 0.15;
    scale[1] = 0.30;
    scale[2] = 0.15;
    scale[3] = 0;
}

void on_update_zaxis(Drive::Robot &bot, const double sliders[4], bool)
{
    static const double vector[4] = {-0.4558, 0.5406, -0.5406, 0.4558};
    double output[G_N_ELEMENTS(vector)];
    for (unsigned int i = 0; i < G_N_ELEMENTS(output); ++i)
    {
        output[i] = sliders[0] * vector[i];
    }
    int w[G_N_ELEMENTS(output)];
    for (unsigned int i = 0; i < G_N_ELEMENTS(w); ++i)
    {
        w[i] = clamp_symmetric(static_cast<int>(output[i]), 255);
    }
    bot.direct_wheels(w);
}

void get_low_sensitivity_scale_factors_zaxis(double scale[4])
{
    scale[0] = scale[1] = scale[2] = scale[3] = 0.1;
}

void on_execute_move_point(
    Drive::Robot &bot, const double sliders[4], bool exact_speed)
{
    if (exact_speed)
    {
        bot.move_move({sliders[0], sliders[1]}, sliders[2]);
    }
    else
    {
        bot.move_move({sliders[0], sliders[1]});
    }
}

void on_execute_move_point_angle(
    Drive::Robot &bot, const double sliders[4], bool exact_speed)
{
    if (exact_speed)
    {
        bot.move_move(
            {sliders[0], sliders[1]}, Angle::of_degrees(sliders[2]),
            sliders[3]);
    }
    else
    {
        bot.move_move({sliders[0], sliders[1]}, Angle::of_degrees(sliders[2]));
    }
}

void on_execute_dribble(
    Drive::Robot &bot, const double sliders[4], bool small_kick_allowed)
{
    bot.move_dribble(
        {sliders[0], sliders[1]}, Angle::of_degrees(sliders[2]), sliders[3],
        small_kick_allowed);
}

void on_execute_shoot_kick(
    Drive::Robot &bot, const double sliders[4], bool exact_angle)
{
    if (exact_angle)
    {
        bot.move_shoot(
            {sliders[0], sliders[1]}, Angle::of_degrees(sliders[2]), sliders[3],
            false);
    }
    else
    {
        bot.move_shoot({sliders[0], sliders[1]}, sliders[3], false);
    }
}

void on_execute_shoot_chip(
    Drive::Robot &bot, const double sliders[4], bool exact_angle)
{
    if (exact_angle)
    {
        bot.move_shoot(
            {sliders[0], sliders[1]}, Angle::of_degrees(sliders[2]), sliders[3],
            true);
    }
    else
    {
        bot.move_shoot({sliders[0], sliders[1]}, sliders[3], true);
    }
}

void on_execute_catch(Drive::Robot &bot, const double sliders[4], bool)
{
    bot.move_catch(Angle::of_degrees(sliders[2]), sliders[1], sliders[0]);
}

void on_execute_pivot(Drive::Robot &bot, const double sliders[4], bool)
{
    bot.move_pivot(
        {sliders[0], sliders[1]}, Angle::of_degrees(sliders[2]),
        Angle::of_degrees(sliders[3]));
}

void on_execute_spin(Drive::Robot &bot, const double sliders[4], bool)
{
    bot.move_spin({sliders[0], sliders[1]}, Angle::of_degrees(sliders[2]));
}

struct SliderInfo final
{
    const char *label;
    double value, min, max, step, page;
    int digits;
};

struct Mode final
{
    const char *name;
    SliderInfo sliders[4];
    bool direct;
    const char *checkbox_label;
    void (*on_update_or_execute)(
        Drive::Robot &bot, const double sliders[4], bool checkbox);
    void (*get_low_sensitivity_scale_factors)(double scale[4]);
};

const Mode MODES[] = {
    {u8"Coast",
     {
         {nullptr, 0, 0, 0, 0, 0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
     },
     false,
     nullptr,
     &on_execute_coast,
     nullptr},
    {u8"Brake",
     {
         {nullptr, 0, 0, 0, 0, 0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
     },
     false,
     nullptr,
     &on_execute_brake,
     nullptr},
    {
        u8"Per-motor",
        {
            {u8"Wheel 0 (PWM)", 0.0, -255.0, 255.0, 1.0, 10.0, 0},
            {u8"Wheel 1 (PWM)", 0.0, -255.0, 255.0, 1.0, 10.0, 0},
            {u8"Wheel 2 (PWM)", 0.0, -255.0, 255.0, 1.0, 10.0, 0},
            {u8"Wheel 3 (PWM)", 0.0, -255.0, 255.0, 1.0, 10.0, 0},
        },
        true,
        nullptr,
        &on_update_permotor,
        nullptr,
    },
    {u8"Matrix",
     {
         {u8"X (m/s)", 0.0, -2.0, 2.0, 0.001, 0.1, 3},
         {u8"Y (m/s)", 0.0, -1.0, 1.0, 0.001, 0.1, 3},
         {u8"θ (°/s)", 0.0, -600.0, 600.0, 1, 10.0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
     },
     true,
     nullptr,
     &on_update_matrix,
     &get_low_sensitivity_scale_factors_matrix},
    {u8"Z axis",
     {
         {u8"Z", 0.0, -2048.0, 2048.0, 1.0, 10.0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
     },
     true,
     nullptr,
     &on_update_zaxis,
     &get_low_sensitivity_scale_factors_zaxis},
    {u8"Move(Point)",
     {
         {u8"X (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"Y (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"t (s)", 0.0, 0.0, 10.0, 0.001, 0.1, 3},
         {nullptr, 0, 0, 0, 0, 0, 0},
     },
     false,
     u8"Speed to end at",
     &on_execute_move_point,
     nullptr},
    {u8"Move(Point, Angle)",
     {
         {u8"X (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"Y (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"θ (°)", 0.0, -360.0, 360.0, 0.5, 50, 1},
         {u8"t (s)", 0.0, 0.0, 10.0, 0.001, 0.1, 3},
     },
     false,
     u8"Speed to end at",
     &on_execute_move_point_angle,
     nullptr},
    {u8"Dribble",
     {
         {u8"X (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"Y (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"θ (°)", 0.0, -360.0, 360.0, 0.5, 50, 1},
         {u8"Speed (RPM)", 0, 0, 40000, 10, 1000, 1},
     },
     false,
     u8"Small Kick Allowed",
     &on_execute_dribble,
     nullptr},
    {u8"Shoot Kick",
     {
         {u8"X (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"Y (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"θ (°)", 0.0, -360.0, 360.0, 0.5, 50, 1},
         {u8"P (m/s)", 0.0, 0.0, 8.0, 0.001, 1, 3},
     },
     false,
     u8"Match Angle",
     &on_execute_shoot_kick,
     nullptr},
    {u8"Shoot Chip",
     {
         {u8"X (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"Y (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"θ (°)", 0.0, -360.0, 360.0, 0.5, 50, 1},
         {u8"P (m)", 0.0, 0.0, 10.0, 0.001, 1, 3},
     },
     false,
     u8"Match Angle",
     &on_execute_shoot_chip,
     nullptr},
    {u8"Catch",
     {
         {u8"X (m/s)", 0.0, -2.0, 2.0, 0.001, 0.1, 3},
         {u8"Y (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"θ (°)", 0.0, -360.0, 360.0, 0.5, 50, 1},
         {nullptr, 0, 0, 0, 0, 0, 0},
     },
     false,
     nullptr,
     &on_execute_catch,
     nullptr},
    {u8"Pivot",
     {
         {u8"X (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"Y (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"Sw (°)", 0.0, -360.0, 360.0, 0.5, 50, 1},
         {u8"θ (°)", 0.0, -360.0, 360.0, 0.5, 50, 1},
     },
     false,
     nullptr,
     &on_execute_pivot,
     nullptr},
    {u8"Spin",
     {
         {u8"X (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"Y (m)", 0.0, -10.0, 10.0, 0.001, 0.1, 3},
         {u8"θ (°/s)", 0.0, -600.0, 600.0, 1, 10.0, 0},
         {nullptr, 0, 0, 0, 0, 0, 0},
     },
     false,
     nullptr,
     &on_execute_spin,
     nullptr},
};
}

DrivePanel::DrivePanel(Drive::Robot &robot)
    : robot(robot),
      scales_table(G_N_ELEMENTS(scales), 2, false),
      misc_checkbox(u8"N/A"),
      execute_button(u8"Execute")
{
    for (const Mode &mode : MODES)
    {
        mode_chooser.append(mode.name);
    }
    mode_chooser.signal_changed().connect(
        sigc::mem_fun(this, &DrivePanel::on_mode_changed));
    pack_start(mode_chooser, Gtk::PACK_SHRINK);
    for (unsigned int i = 0; i != G_N_ELEMENTS(scales); ++i)
    {
        scales_labels[i].set_text(u8"N/A");
        scales[i].set_sensitive(false);
        scales[i].signal_value_changed().connect(
            sigc::mem_fun(this, &DrivePanel::on_update));
        scales_table.attach(
            scales_labels[i], 0, 1, i, i + 1, Gtk::FILL, Gtk::FILL);
        scales_table.attach(
            scales[i], 1, 2, i, i + 1, Gtk::EXPAND | Gtk::FILL, Gtk::FILL);
    }
    pack_start(scales_table, Gtk::PACK_SHRINK);
    misc_checkbox.set_sensitive(false);
    misc_checkbox.signal_toggled().connect(
        sigc::mem_fun(this, &DrivePanel::on_update));
    pack_start(misc_checkbox, Gtk::PACK_SHRINK);
    execute_button.signal_clicked().connect(
        sigc::mem_fun(this, &DrivePanel::on_execute));
    pack_start(execute_button, Gtk::PACK_SHRINK);
    coast();
    on_mode_changed();
}

void DrivePanel::zero()
{
    for (Gtk::HScale &scale : scales)
    {
        scale.get_adjustment()->set_value(0);
    }
}

void DrivePanel::coast()
{
    mode_chooser.set_active(0);
    on_execute();
}

void DrivePanel::set_values(const double values[4])
{
    for (unsigned int i = 0; i < G_N_ELEMENTS(scales); ++i)
    {
        if (scales[i].get_sensitive())
        {
            Glib::RefPtr<Gtk::Adjustment> adj = scales[i].get_adjustment();
            double mid  = (adj->get_lower() + adj->get_upper()) / 2.0;
            double dist = adj->get_upper() - mid;
            adj->set_value(values[i] * dist + mid);
        }
    }
}

void DrivePanel::get_low_sensitivity_scale_factors(double scale[4])
{
    int row = mode_chooser.get_active_row_number();
    if (row >= 0)
    {
        void (*fn)(double scale[4]) =
            MODES[row].get_low_sensitivity_scale_factors;
        if (fn)
        {
            fn(scale);
        }
        else
        {
            scale[0] = scale[1] = scale[2] = scale[3] = 1.0;
        }
    }
}

void DrivePanel::on_mode_changed()
{
    int row = mode_chooser.get_active_row_number();
    if (row >= 0)
    {
        robot.direct_control = MODES[row].direct;
        for (unsigned int i = 0; i < G_N_ELEMENTS(scales); ++i)
        {
            const SliderInfo &info = MODES[row].sliders[i];
            scales_labels[i].set_text(info.label ? info.label : u8"N/A");
            scales[i].set_sensitive(!!info.label);
            scales[i].get_adjustment()->configure(
                info.value, info.min, info.max, info.step, info.page, 0);
            scales[i].set_digits(info.digits);
        }
        misc_checkbox.set_sensitive(!!MODES[row].checkbox_label);
        misc_checkbox.set_label(
            MODES[row].checkbox_label ? MODES[row].checkbox_label : u8"N/A");
        execute_button.set_sensitive(!MODES[row].direct);
        on_update();
    }
}

void DrivePanel::on_update()
{
    int row = mode_chooser.get_active_row_number();
    if (row >= 0)
    {
        if (MODES[row].direct)
        {
            double values[G_N_ELEMENTS(scales)];
            for (unsigned int i = 0; i != G_N_ELEMENTS(scales); ++i)
            {
                values[i] = scales[i].get_adjustment()->get_value();
            }
            MODES[row].on_update_or_execute(robot, values, false);
        }
    }
}

void DrivePanel::on_execute()
{
    int row = mode_chooser.get_active_row_number();
    if (row >= 0)
    {
        double values[G_N_ELEMENTS(scales)];
        for (unsigned int i = 0; i != G_N_ELEMENTS(scales); ++i)
        {
            values[i] = scales[i].get_adjustment()->get_value();
        }
        MODES[row].on_update_or_execute(
            robot, values, misc_checkbox.get_active());
    }
}
