#include "test/common/dribble.h"
#include "util/algorithm.h"
#include <gtkmm/adjustment.h>
#include <sigc++/functors/mem_fun.h>

DribblePanel::DribblePanel(Drive::Robot &robot) :
		robot(robot),
		dribble_button(u8"Run") {
	robot.direct_control.signal_changed().connect(sigc::mem_fun(this, &DribblePanel::on_direct_changed));
	dribble_button.signal_toggled().connect(sigc::mem_fun(this, &DribblePanel::on_update));
	pack_start(dribble_button, Gtk::PACK_SHRINK);
	level.get_adjustment()->configure(0.0, 0.0, robot.direct_dribbler_max, 300, 900, 0.0);
	level.set_digits(0);
	level.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &DribblePanel::on_update));
	pack_start(level, Gtk::PACK_SHRINK);
	on_direct_changed();
}

void DribblePanel::stop() {
	dribble_button.set_active(false);
}

void DribblePanel::toggle() {
	dribble_button.set_active(!dribble_button.get_active());
}

void DribblePanel::on_direct_changed() {
	dribble_button.set_sensitive(robot.direct_control);
	level.set_sensitive(robot.direct_control);
	on_update();
}

void DribblePanel::on_update() {
	if (robot.direct_control) {
		robot.direct_dribbler(dribble_button.get_active() ? static_cast<unsigned int>(level.get_value()) : 0U);
	}
}
