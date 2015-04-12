#include "ai/backend/backend.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>
#include <cstdlib>

using AI::BE::Backend;
using AI::BE::BackendFactory;

DoubleParam AI::BE::LOOP_DELAY(u8"Loop Delay", u8"AI/Backend", 0.08, -1.0, 1.0);

AI::BE::Backend::~Backend() = default;

const AI::BE::Field &AI::BE::Backend::field() const {
	return field_;
}

const AI::BE::Ball &AI::BE::Backend::ball() const {
	return ball_;
}

std::size_t Backend::visualizable_num_robots() const {
	return friendly_team().size() + enemy_team().size();
}

Visualizable::Robot::Ptr Backend::visualizable_robot(std::size_t index) const {
	if (index < friendly_team().size()) {
		return friendly_team().get(index);
	} else {
		return enemy_team().get(index - friendly_team().size());
	}
}

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

sigc::signal<void> &AI::BE::Backend::signal_tick() const {
	return signal_tick_;
}

void Backend::mouse_pressed(Point, unsigned int) {
}

void Backend::mouse_released(Point, unsigned int) {
}

void Backend::mouse_exited() {
}

void Backend::mouse_moved(Point) {
}

Backend::Backend() : defending_end_(FieldEnd::WEST), friendly_colour_(AI::Common::Colour::YELLOW), playtype_(AI::Common::PlayType::HALT), playtype_override_(AI::Common::PlayType::NONE) {
	monotonic_time_ = monotonic_start_time_ = std::chrono::steady_clock::now();
}

void Backend::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
	signal_draw_overlay_.emit(ctx);
}

BackendFactory::BackendFactory(const char *name) : Registerable<BackendFactory>(name) {
}

