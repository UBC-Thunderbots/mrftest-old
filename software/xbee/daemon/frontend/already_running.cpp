#include "xbee/daemon/frontend/already_running.h"

AlreadyRunning::AlreadyRunning() : std::runtime_error("An XBee arbiter is already running.") {
}

AlreadyRunning::~AlreadyRunning() throw () {
}

