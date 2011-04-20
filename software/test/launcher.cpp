#include "test/launcher.h"
#include <cstddef>

TesterLauncher::TesterLauncher(XBeeDongle &dongle) : dongle(dongle), table(4, 4, true) {
	set_title("Tester");
	for (unsigned int i = 0; i < G_N_ELEMENTS(checkboxes); ++i) {
		checkboxes[i].set_label(Glib::ustring::format(i));
		checkboxes[i].signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &TesterLauncher::on_checkbox_toggled), i));
		table.attach(checkboxes[i], i % 4, i % 4 + 1, i / 4, i / 4 + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
	}
	vbox.pack_start(table, Gtk::PACK_SHRINK);
	vbox.pack_start(ann, Gtk::PACK_EXPAND_WIDGET);
	add(vbox);
	show_all();
}

void TesterLauncher::on_checkbox_toggled(unsigned int index) {
	if (checkboxes[index].get_active()) {
		windows[index].reset(new TesterWindow(dongle, dongle.robot(index)));
		windows[index]->signal_delete_event().connect(sigc::bind_return(sigc::hide(sigc::bind(sigc::mem_fun(&checkboxes[index], &Gtk::CheckButton::set_active), false)), false));
	} else {
		windows[index].reset();
	}
}

