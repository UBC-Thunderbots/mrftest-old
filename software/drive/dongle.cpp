#include "drive/dongle.h"
#include <sigc++/functors/mem_fun.h>

Drive::Dongle::~Dongle() = default;

Drive::Dongle::Dongle() : estop_state(EStopState::STOP), estop_broken_message(u8"EStop missing/broken", Annunciator::Message::TriggerMode::LEVEL) {
	estop_state.signal_changed().connect(sigc::mem_fun(this, &Dongle::handle_estop_state_changed));
}

void Drive::Dongle::handle_estop_state_changed() {
	estop_broken_message.active(estop_state == EStopState::BROKEN);
}

