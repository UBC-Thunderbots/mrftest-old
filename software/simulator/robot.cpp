#include "simulator/robot.h"

RefPtr<SimulatorRobot> SimulatorRobot::create(const Config::RobotInfo &botinfo, RefPtr<SimulatorEngine> engine) {
	RefPtr<SimulatorRobot> p(new SimulatorRobot(botinfo, engine));
	return p;
}

SimulatorRobot::SimulatorRobot(const Config::RobotInfo &botinfo, RefPtr<SimulatorEngine> engine) : address(botinfo.address), engine(engine), botinfo(botinfo), powered_(false), battery_(15.0), bootloading_(false), address16_(0xFFFF), run_data_offset_(0xFF) {
}

void SimulatorRobot::powered(bool pwr) {
	if (pwr != powered_) {
		powered_ = pwr;
		if (!powered_) {
			// Switching off the power loses all on-board state.
			bootloading_ = false;
			address16_ = 0xFFFF;
			run_data_offset_ = 0xFF;
		}
		signal_changed.emit();
	}
}

void SimulatorRobot::battery(double bat) {
	if (bat != battery_) {
		battery_ = bat;
		signal_changed.emit();
	}
}

void SimulatorRobot::bootloading(bool bl) {
	if (bl != bootloading_) {
		bootloading_ = bl;
		if (bootloading_) {
			// The transition into bootloader mode will result in the PIC
			// shutting down operation of the FPGA, which causes the FPGA to
			// its on-board state consisting of the run data offset.
			run_data_offset_ = 0xFF;
		} else {
			// The transition out of bootloader mode will result in the PIC
			// pulsing the XBee's RESET line, which causes the XBee to lose its
			// on-board state consisting of its 16-bit address.
			address16_ = 0xFFFF;
		}
		signal_changed.emit();
	}
}

void SimulatorRobot::address16(uint16_t addr) {
	if (addr != address16_) {
		address16_ = addr;
		signal_changed.emit();
	}
}

void SimulatorRobot::run_data_offset(uint8_t offset) {
	if (offset != run_data_offset_) {
		run_data_offset_ = offset;
		signal_changed.emit();
	}
}

void SimulatorRobot::add_player() {
	if (!player_) {
		player_ = engine->add_player();
		signal_changed.emit();
	}
}

void SimulatorRobot::remove_player() {
	if (player_) {
		engine->remove_player(player_);
		player_.reset();
		signal_changed.emit();
	}
}

