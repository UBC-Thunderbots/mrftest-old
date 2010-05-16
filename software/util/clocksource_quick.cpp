#include "util/clocksource_quick.h"
#include <glibmm.h>

bool clocksource_quick::on_idle() {
	signal_tick.emit();
	return true;
}

void clocksource_quick::start() {
	connection.disconnect();
	connection = Glib::signal_idle().connect(sigc::mem_fun(this, &clocksource_quick::on_idle));
}

void clocksource_quick::stop() {
	connection.disconnect();
}

