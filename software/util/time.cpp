#include "util/time.h"
#include <glibmm.h>

timeout::timeout(unsigned int period) : period(period) {
}

timeout::~timeout() {
}

void timeout::start() {
	if (!conn.connected()) {
		conn = Glib::signal_timeout().connect(sigc::mem_fun(this, &timeout::on_tick), period);
	}
}

void timeout::stop() {
	conn.disconnect();
}

bool timeout::on_tick() {
	conn.disconnect();
	signal_expired.emit();
	return false;
}

