#include "test/params.h"
#include <cstddef>
#include <iomanip>
#include <sstream>

namespace {
	Glib::ustring format_channel(unsigned int ch) {
		return Glib::ustring::format(std::setfill(L'0'), std::setw(2), std::hex, std::uppercase, ch);
	}
}

TesterParamsPanel::TesterParamsPanel() : Gtk::Table(4, 2), commit("Commit"), rollback("Rollback"), reboot("Reboot"), set_test_mode("Set Test Mode"), freeze(false) {
	for (std::size_t i = 0; i < 2; ++i) {
		for (unsigned int ch = 0x0B; ch <= 0x1A; ++ch) {
			channels[i].append_text(format_channel(ch));
		}
		channels[i].signal_changed().connect(sigc::mem_fun(this, &TesterParamsPanel::on_change));
	}
	for (unsigned int i = 1; i <= 15; ++i) {
		index.append_text(Glib::ustring::format(i));
	}
	index.signal_changed().connect(sigc::mem_fun(this, &TesterParamsPanel::on_change));
	commit.signal_clicked().connect(sigc::mem_fun(this, &TesterParamsPanel::on_commit));
	rollback.signal_clicked().connect(sigc::mem_fun(this, &TesterParamsPanel::on_rollback));
	reboot.signal_clicked().connect(sigc::mem_fun(this, &TesterParamsPanel::on_reboot));
	set_test_mode.signal_clicked().connect(sigc::mem_fun(this, &TesterParamsPanel::on_set_test_mode));

	attach(*Gtk::manage(new Gtk::Label("Out Channel:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(channels[0], 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(*Gtk::manage(new Gtk::Label("In Channel:")), 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(channels[1], 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(*Gtk::manage(new Gtk::Label("Index:")), 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	attach(index, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	Gtk::HButtonBox *hbb = Gtk::manage(new Gtk::HButtonBox);
	hbb->pack_start(commit);
	hbb->pack_start(rollback);
	hbb->pack_start(reboot);
	vbox->pack_start(*hbb, Gtk::PACK_SHRINK);
	Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox);
	hbox->pack_start(*Gtk::manage(new Gtk::Label("Test mode (hex):")), Gtk::PACK_SHRINK);
	hbox->pack_start(test_mode, Gtk::PACK_EXPAND_WIDGET);
	hbox->pack_start(set_test_mode);
	vbox->pack_start(*hbox, Gtk::PACK_SHRINK);
	attach(*vbox, 0, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

	set_robot(XBeeRobot::Ptr());
}

TesterParamsPanel::~TesterParamsPanel() {
}

void TesterParamsPanel::set_robot(XBeeRobot::Ptr bot) {
	alive_connection.disconnect();
	robot = bot;
	if (robot.is()) {
		alive_connection = robot->alive.signal_changed().connect(sigc::mem_fun(this, &TesterParamsPanel::on_alive_changed));
	}
	on_alive_changed();
}

void TesterParamsPanel::activate_controls(bool act) {
	channels[0].set_sensitive(act);
	channels[1].set_sensitive(act);
	index.set_sensitive(act);
	commit.set_sensitive(act);
	rollback.set_sensitive(act);
	reboot.set_sensitive(act);
}

void TesterParamsPanel::on_alive_changed() {
	activate_controls(false);
	if (robot.is() && robot->alive) {
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
		params.robot_number = static_cast<uint8_t>(1 + index.get_active_row_number());
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

void TesterParamsPanel::on_rollback() {
#warning implement
	Gtk::MessageDialog md(*dynamic_cast<Gtk::Window *>(get_toplevel()), "Not implemented", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	md.run();
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
	std::istringstream iss(test_mode.get_text());
	unsigned int mode;
	iss >> std::hex >> mode;
	if (mode < 255) {
		robot->test_mode(mode);
	}
}

