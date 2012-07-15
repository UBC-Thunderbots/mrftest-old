#include "drive/dongle.h"

Drive::Dongle::~Dongle() = default;

Drive::Dongle::Dongle() : estop_state(EStopState::STOP) {
}

