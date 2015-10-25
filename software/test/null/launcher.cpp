#include "test/null/launcher.h"
#include <algorithm>
#include <cstddef>

NullTesterLauncher::NullTesterLauncher(Drive::Null::Dongle &dongle) :
		dongle(dongle),
		robot_toggle(u8"Show Robot Window"),
		mapper_toggle(u8"Joystick Mapper") {
	set_title(u8"Tester");
	robot_toggle.signal_toggled().connect(sigc::mem_fun(this, &NullTesterLauncher::on_robot_toggled));
	vbox.pack_start(robot_toggle, Gtk::PACK_SHRINK);
	mapper_toggle.signal_toggled().connect(sigc::mem_fun(this, &NullTesterLauncher::on_mapper_toggled));
	vbox.pack_start(mapper_toggle, Gtk::PACK_SHRINK);
	vbox.pack_start(ann, Gtk::PACK_EXPAND_WIDGET);
	add(vbox);
	show_all();
}

void NullTesterLauncher::on_robot_toggled() {
	if (robot_toggle.get_active()) {
		mapper_toggle.set_active(false);
		window.reset(new NullTesterWindow(dongle, dongle.robot(0)));
		window->signal_delete_event().connect(sigc::bind_return(sigc::hide(sigc::bind(sigc::mem_fun(&robot_toggle, &Gtk::ToggleButton::set_active), false)), false));
	} else {
		window.reset();
	}
}

void NullTesterLauncher::on_mapper_toggled() {
	if (mapper_toggle.get_active()) {
		robot_toggle.set_active(false);
		mapper_window.reset(new MapperWindow);
		mapper_window->signal_delete_event().connect(sigc::bind_return(sigc::hide(sigc::bind(sigc::mem_fun(&mapper_toggle, &Gtk::ToggleButton::set_active), false)), false));
	} else {
		mapper_window.reset();
	}
}
