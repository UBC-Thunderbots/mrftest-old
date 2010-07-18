#include "util/time.h"
#include <glibmm.h>

Timeout::Timeout(unsigned int period) : period(period) {
}

Timeout::~Timeout() {
}

void Timeout::start() {
	if (!conn.connected()) {
		conn = Glib::signal_timeout().connect(sigc::mem_fun(this, &Timeout::on_tick), period);
	}
}

void Timeout::stop() {
	conn.disconnect();
}

bool Timeout::on_tick() {
	conn.disconnect();
	signal_expired.emit();
	return false;
}

