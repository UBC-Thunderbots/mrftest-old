#include "test/params.h"
#include "util/dprint.h"
#include <cstddef>
#include <iomanip>
#include <sstream>

namespace {
	Glib::ustring format_channel(unsigned int ch) {
		return Glib::ustring::format(std::setfill(L'0'), std::setw(2), std::hex, std::uppercase, ch);
	}
}

TesterParamsPanel::TesterParamsPanel(XBeeRobot::Ptr robot) : Gtk::Table(6, 2), robot(robot), channel0label("Out Channel:"), channel1label("In Channel:"), index_label("Index:"), dribble_power_label("Dribble Power:"), commit("Commit"), reboot("Reboot"), test_mode_label("Test mode (hex):"), set_test_mode("Set Test Mode"), build_signatures_label("Build Sigs:"), freeze(false) {
	for (std::size_t i = 0; i < 2; ++i) {
		for (unsigned int ch = 0x0B; ch <= 0x1A; ++ch) {
			channels[i].append_text(format_channel(ch));
		}
		channels[i].signal_changed().connect(sigc::mem_fun(this, &TesterParamsPanel::on_change));
	}
	for (unsigned int i = 0; i <= 15; ++i) {
		index.append_text(Glib::ustring::format(i));
	}
	dribble_power.get_adjustment()->configure(0, 0, 255, 1, 10, 0);
	dribble_power.set_digits(0);
	index.signal_changed().connect(sigc::mem_fun(this, &TesterParamsPanel::on_change));
	dribble_power.signal_value_changed().connect(sigc::mem_fun(this, &TesterParamsPanel::on_change));
	commit.signal_clicked().connect(sigc::mem_fun(this, &TesterParamsPanel::on_commit));
	reboot.signal_clicked().connect(sigc::mem_fun(this, &TesterParamsPanel::on_reboot));
	test_mode.set_max_length(2);
	test_mode.set_width_chars(2);
	set_test_mode.signal_clicked().connect(sigc::mem_fun(this, &TesterParamsPanel::on_set_test_mode));

	attach(channel0label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(channels[0], 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(channel1label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(channels[1], 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(index_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(index, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(dribble_power_label, 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(dribble_power, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	hbb.pack_start(commit);
	hbb.pack_start(reboot);
	vbox.pack_start(hbb, Gtk::PACK_SHRINK);
	test_mode_hbox.pack_start(test_mode_label, Gtk::PACK_SHRINK);
	test_mode_hbox.pack_start(test_mode, Gtk::PACK_EXPAND_WIDGET);
	test_mode_hbox.pack_start(set_test_mode);
	vbox.pack_start(test_mode_hbox, Gtk::PACK_SHRINK);
	attach(vbox, 0, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	build_signatures_hbox.pack_start(firmware_signature_label, Gtk::PACK_EXPAND_WIDGET);
	build_signatures_hbox.pack_start(flash_signature_label, Gtk::PACK_EXPAND_WIDGET);
	attach(build_signatures_label, 0, 1, 5, 6, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(build_signatures_hbox, 1, 2, 5, 6, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	robot->alive.signal_changed().connect(sigc::mem_fun(this, &TesterParamsPanel::on_alive_changed));
	on_alive_changed();
}

void TesterParamsPanel::activate_controls(bool act) {
	channels[0].set_sensitive(act);
	channels[1].set_sensitive(act);
	index.set_sensitive(act);
	commit.set_sensitive(act);
	reboot.set_sensitive(act);
}

void TesterParamsPanel::on_alive_changed() {
	activate_controls(false);
	if (robot->alive) {
		robot->firmware_read_build_signatures()->signal_done.connect(sigc::mem_fun(this, &TesterParamsPanel::on_read_build_signatures_done));
	}
}

void TesterParamsPanel::on_read_build_signatures_done(AsyncOperation<XBeeRobot::BuildSignatures>::Ptr op) {
	const XBeeRobot::BuildSignatures &sigs = op->result();
	firmware_signature_label.set_text(Glib::ustring::compose("PIC Firmware: 0x%1", tohex(sigs.firmware_signature, 4)));
	flash_signature_label.set_text(Glib::ustring::compose("SPI Flash: 0x%1", tohex(sigs.flash_signature, 4)));
	if (robot->alive) {
		robot->firmware_read_operational_parameters()->signal_done.connect(sigc::mem_fun(this, &TesterParamsPanel::on_read_done));
	}
}

void TesterParamsPanel::on_read_done(AsyncOperation<XBeeRobot::OperationalParameters>::Ptr op) {
	const XBeeRobot::OperationalParameters &params = op->result();
	freeze = true;
	flash_contents = params.flash_contents;
	for (std::size_t i = 0; i < 2; ++i) {
		channels[i].set_active_text(format_channel(params.xbee_channels[i]));
	}
	index.set_active_text(Glib::ustring::format(static_cast<unsigned int>(params.robot_number)));
	dribble_power.set_value(params.dribble_power);
	activate_controls();
	freeze = false;
}

void TesterParamsPanel::on_change() {
	if (!freeze) {
		activate_controls(false);
		XBeeRobot::OperationalParameters params;
		params.flash_contents = flash_contents;
		for (std::size_t i = 0; i < 2; ++i) {
			params.xbee_channels[i] = static_cast<uint8_t>(0x0B + channels[i].get_active_row_number());
		}
		params.robot_number = static_cast<uint8_t>(index.get_active_row_number());
		params.dribble_power = static_cast<uint8_t>(dribble_power.get_value());
		robot->firmware_set_operational_parameters(params)->signal_done.connect(sigc::mem_fun(this, &TesterParamsPanel::on_change_done));
	}
}

void TesterParamsPanel::on_change_done(AsyncOperation<void>::Ptr op) {
	op->result();
	activate_controls();
}

void TesterParamsPanel::on_commit() {
	activate_controls(false);
	robot->firmware_commit_operational_parameters()->signal_done.connect(sigc::mem_fun(this, &TesterParamsPanel::on_commit_done));
}

void TesterParamsPanel::on_commit_done(AsyncOperation<void>::Ptr op) {
	op->result();
	activate_controls();
}

void TesterParamsPanel::on_reboot() {
	activate_controls(false);
	robot->firmware_reboot()->signal_done.connect(sigc::mem_fun(this, &TesterParamsPanel::on_reboot_done));
}

void TesterParamsPanel::on_reboot_done(AsyncOperation<void>::Ptr op) {
	op->result();
	activate_controls();
}

void TesterParamsPanel::on_set_test_mode() {
	const Glib::ustring &text = test_mode.get_text();
	if (!(text.size() == 2 && text[0] >= '0' && text[0] <= '7' && text[1] >= '0' && text[1] <= '7')) {
		Gtk::MessageDialog md(*dynamic_cast<Gtk::Window *>(get_toplevel()), "Invalid test mode", false, Gtk::MESSAGE_ERROR);
		md.run();
		return;
	}
	std::istringstream iss(test_mode.get_text());
	unsigned int mode;
	iss >> std::hex >> mode;
	if (mode < 255) {
		robot->test_mode(mode);
	}
}

