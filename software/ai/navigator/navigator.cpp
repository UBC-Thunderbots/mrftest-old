#include "ai/navigator/navigator.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

Gtk::Widget *Navigator::ui_controls() {
	return 0;
}

void Navigator::draw_overlay(Cairo::RefPtr<Cairo::Context>) {
}

Navigator::Navigator(World &world) : world(world) {
}

Navigator::~Navigator() = default;

NavigatorFactory::NavigatorFactory(const char *name) : Registerable<NavigatorFactory>(name) {
}

