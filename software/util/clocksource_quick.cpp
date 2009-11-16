#include "util/clocksource_quick.h"
#include <glibmm.h>

clocksource_quick::clocksource_quick() {
	Glib::signal_idle().connect(sigc::mem_fun(*this, &clocksource_quick::on_idle));
}

bool clocksource_quick::on_idle() {
	signal_tick().emit();
	return true;
}

