#include "ai/backend/backend.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>
#include <cstdlib>

#warning const-correctness is broken in AI::BE::Robot::Ptr.

using AI::BE::Backend;
using AI::BE::BackendFactory;

DoubleParam AI::BE::LOOP_DELAY("Loop Delay", "Backend", 0.08, -1.0, 1.0);

unsigned int Backend::main_ui_controls_table_rows() const {
	return 0;
}

void Backend::main_ui_controls_attach(Gtk::Table &, unsigned int) {
}

unsigned int Backend::secondary_ui_controls_table_rows() const {
	return 0;
}

void Backend::secondary_ui_controls_attach(Gtk::Table &, unsigned int) {
}

Backend::Backend() : defending_end_(FieldEnd::WEST), friendly_colour_(AI::Common::Colour::YELLOW), playtype_(AI::Common::PlayType::HALT), playtype_override_(AI::Common::PlayType::NONE), ball_filter_(0) {
	monotonic_time_.tv_sec = 0;
	monotonic_time_.tv_nsec = 0;
}

void Backend::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
	signal_draw_overlay_.emit(ctx);
}

BackendFactory::BackendFactory(const char *name) : Registerable<BackendFactory>(name) {
}

