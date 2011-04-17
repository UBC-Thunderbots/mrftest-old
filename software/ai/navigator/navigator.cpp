#include "ai/navigator/navigator.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

Gtk::Widget *Navigator::ui_controls() {
	return 0;
}

Navigator::Navigator(World &world) : world(world) {
}

NavigatorFactory::NavigatorFactory(const char *name) : Registerable<NavigatorFactory>(name) {
}

