#include "util/clocksource_quick.h"
#include <glibmm.h>

bool QuickClockSource::on_idle() {
	signal_tick.emit();
	return true;
}

void QuickClockSource::start() {
	connection.disconnect();
	connection = Glib::signal_idle().connect(sigc::mem_fun(this, &QuickClockSource::on_idle));
}

void QuickClockSource::stop() {
	connection.disconnect();
}

