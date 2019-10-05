#include "test/mrf/launcher.h"
#include <algorithm>
#include <cstddef>

using namespace std::placeholders;

TesterLauncher::TesterLauncher(MRFDongle &dongle)
    : dongle(dongle), table(3, 4, true), mapper_toggle(u8"Joystick Mapper")
{
    set_title(u8"Tester");
    for (unsigned int i = 0; i < G_N_ELEMENTS(robot_toggles); ++i)
    {
        robot_toggles[i].set_label(Glib::ustring::format(i));
        robot_toggles[i].signal_toggled().connect(sigc::bind(
            sigc::mem_fun(this, &TesterLauncher::on_robot_toggled), i));
        table.attach(
            robot_toggles[i], i % 4, i % 4 + 1, i / 4, i / 4 + 1,
            Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
    }
    mapper_toggle.signal_toggled().connect(
        sigc::mem_fun(this, &TesterLauncher::on_mapper_toggled));
    table.attach(
        mapper_toggle, 0, 4, 2, 3, Gtk::FILL | Gtk::EXPAND,
        Gtk::FILL | Gtk::SHRINK);
    vbox.pack_start(table, Gtk::PACK_SHRINK);
    vbox.pack_start(ann, Gtk::PACK_EXPAND_WIDGET);
    add(vbox);
    show_all();
}

void TesterLauncher::on_robot_toggled(unsigned int index)
{
    if (robot_toggles[index].get_active())
    {
        mapper_toggle.set_active(false);
        windows[index].reset(new TesterWindow(dongle, dongle.robot(index)));
        windows[index]->signal_delete_event().connect(sigc::bind_return(
            sigc::hide(sigc::bind(
                sigc::mem_fun(
                    &robot_toggles[index], &Gtk::ToggleButton::set_active),
                false)),
            false));
    }
    else
    {
        windows[index].reset();
    }
}

void TesterLauncher::on_mapper_toggled()
{
    if (mapper_toggle.get_active())
    {
        std::for_each(
            robot_toggles, robot_toggles + G_N_ELEMENTS(robot_toggles),
            [](Gtk::ToggleButton &btn) { btn.set_active(false); });
        mapper_window.reset(new MapperWindow);
        mapper_window->signal_delete_event().connect(sigc::bind_return(
            sigc::hide(sigc::bind(
                sigc::mem_fun(&mapper_toggle, &Gtk::ToggleButton::set_active),
                false)),
            false));
    }
    else
    {
        mapper_window.reset();
    }
}
