#include "ai/navigator/navigator.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

Navigator::Navigator(World &world) : world(world) {
}

Navigator::~Navigator() {
}

NavigatorFactory::NavigatorFactory(const Glib::ustring &name) : Registerable<NavigatorFactory>(name) {
}

NavigatorFactory::~NavigatorFactory() {
}

