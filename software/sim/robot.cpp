#define DEBUG 0
#include "sim/robot.h"
#include "util/dprint.h"

robot::ptr robot::create(const config::robot_info &botinfo, simulator_engine::ptr engine) {
	ptr p(new robot(botinfo, engine));
	return p;
}

robot::robot(const config::robot_info &botinfo, simulator_engine::ptr engine) : address(botinfo.address), engine(engine), botinfo(botinfo), powered_(false), battery_(15.0), bootloading_(false), address16_(0xFFFF), run_data_offset_(0xFF) {
}

void robot::powered(bool pwr) {
	if (pwr != powered_) {
		DPRINT(Glib::ustring::compose("Robot turning %1.", pwr ? "on" : "off"));
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

void robot::battery(double bat) {
	if (bat != battery_) {
		DPRINT(Glib::ustring::compose("Robot setting battery voltage to %1V.", bat));
		battery_ = bat;
		signal_changed.emit();
	}
}

void robot::bootloading(bool bl) {
	if (bl != bootloading_) {
		DPRINT(Glib::ustring::compose("Robot %1 bootload mode.", bl ? "entering" : "exiting"));
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

void robot::address16(uint16_t addr) {
	if (addr != address16_) {
		DPRINT(Glib::ustring::compose("Robot receiving 16-bit address 0x%1.", tohex(addr, 4)));
		address16_ = addr;
		signal_changed.emit();
	}
}

void robot::run_data_offset(uint8_t offset) {
	if (offset != run_data_offset_) {
		DPRINT(Glib::ustring::compose("Robot receiving run data offset %1.", static_cast<unsigned int>(offset)));
		run_data_offset_ = offset;
		signal_changed.emit();
	}
}

void robot::add_player() {
	if (!player_) {
		DPRINT("Robot being placed on field.");
		player_ = engine->add_player();
		signal_changed.emit();
	}
}

void robot::remove_player() {
	if (player_) {
		DPRINT("Robot being removed from field.");
		engine->remove_player(player_);
		player_.reset();
		signal_changed.emit();
	}
}

