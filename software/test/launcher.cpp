#include "test/launcher.h"
#include <algorithm>
#include <cstddef>
#include <functional>

using namespace std::placeholders;

TesterLauncher::TesterLauncher(XBeeDongle &dongle) : dongle(dongle), table(5, 4, true), mapper_toggle("Joystick Mapper") {
	set_title("Tester");
	for (unsigned int i = 0; i < G_N_ELEMENTS(robot_toggles); ++i) {
		robot_toggles[i].set_label(Glib::ustring::format(i));
		robot_toggles[i].signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &TesterLauncher::on_robot_toggled), i));
		table.attach(robot_toggles[i], i % 4, i % 4 + 1, i / 4, i / 4 + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
	}
	mapper_toggle.signal_toggled().connect(sigc::mem_fun(this, &TesterLauncher::on_mapper_toggled));
	table.attach(mapper_toggle, 0, 4, 4, 5, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::SHRINK);
	vbox.pack_start(table, Gtk::PACK_SHRINK);
	vbox.pack_start(ann, Gtk::PACK_EXPAND_WIDGET);
	add(vbox);
	show_all();
}

void TesterLauncher::on_robot_toggled(unsigned int index) {
	if (robot_toggles[index].get_active()) {
		mapper_toggle.set_active(false);
		windows[index].reset(new TesterWindow(dongle, dongle.robot(index)));
		windows[index]->signal_delete_event().connect(sigc::bind_return(sigc::hide(sigc::bind(sigc::mem_fun(&robot_toggles[index], &Gtk::ToggleButton::set_active), false)), false));
	} else {
		windows[index].reset();
	}
}

void TesterLauncher::on_mapper_toggled() {
	if (mapper_toggle.get_active()) {
		std::for_each(robot_toggles, robot_toggles + G_N_ELEMENTS(robot_toggles), std::bind(std::mem_fn(&Gtk::ToggleButton::set_active), _1, false));
		mapper_window.reset(new MapperWindow);
		mapper_window->signal_delete_event().connect(sigc::bind_return(sigc::hide(sigc::bind(sigc::mem_fun(&mapper_toggle, &Gtk::ToggleButton::set_active), false)), false));
	} else {
		mapper_window.reset();
	}
}

