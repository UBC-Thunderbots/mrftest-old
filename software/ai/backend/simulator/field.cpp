#include "ai/backend/simulator/field.h"
#include <glibmm/main.h>

AI::BE::Simulator::Field::Field() {
	Glib::signal_idle().connect_once(signal_changed.make_slot());
}

