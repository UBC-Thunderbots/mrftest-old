#define DEBUG 0
#include "tester/controlled_permotor_drive.h"
#include "tester/direct_drive.h"
#include "tester/feedback.h"
#include "tester/matrix_drive.h"
#include "tester/window.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>
#include <functional>

tester_window::tester_window(xbee_lowlevel &modem, const config &conf) : modem(modem), bot_frame("Bot"), bot_chooser(conf.robots()), claim_bot_button("Claim"), feedback_frame("Feedback"), drive_frame("Drive"), drive_widget(0), drive_zeroable(0), dribble_frame("Dribble"), dribble_scale(-1023, 1023, 1), chicker_frame("Chicker"), chicker_enabled("Enable"), chicker_kick("Kick"), chicker_chip("Chip") {
	set_title("Robot Tester");

	bot_hbox.pack_start(bot_chooser, Gtk::PACK_EXPAND_WIDGET);
	claim_bot_button.signal_toggled().connect(sigc::mem_fun(this, &tester_window::on_claim_toggled));
	bot_hbox.pack_start(claim_bot_button, Gtk::PACK_SHRINK);
	bot_frame.add(bot_hbox);
	vbox.pack_start(bot_frame, Gtk::PACK_SHRINK);

	feedback_frame.add(feedback);
	vbox.pack_start(feedback_frame, Gtk::PACK_SHRINK);

	drive_chooser.append_text("Halt");
	drive_chooser.append_text("Direct Drive");
	drive_chooser.append_text("Controlled Per-Motor Drive");
	drive_chooser.append_text("Matrix Drive");
	drive_chooser.set_active_text("Halt");
	drive_chooser.set_sensitive(false);
	drive_chooser.signal_changed().connect(sigc::mem_fun(this, &tester_window::drive_mode_changed));
	drive_box.pack_start(drive_chooser, Gtk::PACK_SHRINK);
	drive_frame.add(drive_box);
	vbox.pack_start(drive_frame, Gtk::PACK_SHRINK);

	dribble_scale.get_adjustment()->set_page_size(0);
	dribble_scale.set_value(0);
	dribble_scale.signal_value_changed().connect(sigc::mem_fun(this, &tester_window::on_dribble_change));
	dribble_scale.set_sensitive(false);
	dribble_frame.add(dribble_scale);
	vbox.pack_start(dribble_frame, Gtk::PACK_SHRINK);

	chicker_enabled.signal_toggled().connect(sigc::mem_fun(this, &tester_window::on_chicker_enable_change));
	chicker_enabled.set_sensitive(false);
	chicker_box.pack_start(chicker_enabled, Gtk::PACK_SHRINK);
	chicker_power.get_adjustment()->configure(32, 32, 16384 - 32, 32, 1024, 0);
	chicker_power.set_digits(0);
	chicker_box.pack_start(chicker_power, Gtk::PACK_EXPAND_WIDGET);
	chicker_kick.signal_clicked().connect(sigc::mem_fun(this, &tester_window::on_chicker_kick));
	chicker_kick.set_sensitive(false);
	chicker_box.pack_start(chicker_kick, Gtk::PACK_SHRINK);
	chicker_chip.signal_clicked().connect(sigc::mem_fun(this, &tester_window::on_chicker_chip));
	chicker_chip.set_sensitive(false);
	chicker_box.pack_start(chicker_chip, Gtk::PACK_SHRINK);
	chicker_box.pack_start(chicker_status, Gtk::PACK_SHRINK);
	chicker_frame.add(chicker_box);
	vbox.pack_start(chicker_frame, Gtk::PACK_SHRINK);

	add(vbox);

	Gtk::Main::signal_key_snooper().connect(sigc::mem_fun(this, &tester_window::key_snoop));

	show_all();
}

void tester_window::on_claim_toggled() {
	drive_chooser.set_active_text("Halt");
	dribble_scale.set_value(0);
	chicker_enabled.set_active(false);

	if (claim_bot_button.get_active()) {
		bot = xbee_drive_bot::create(bot_chooser.address(), modem);
		bot->signal_alive.connect(sigc::mem_fun(this, &tester_window::on_bot_alive));
		bot->signal_dead.connect(sigc::mem_fun(this, &tester_window::on_bot_dead));
		bot->signal_claim_failed_locked.connect(sigc::mem_fun(this, &tester_window::on_bot_claim_failed_locked));
		bot->signal_claim_failed_resource.connect(sigc::mem_fun(this, &tester_window::on_bot_claim_failed_resource));
		bot->signal_feedback.connect(sigc::mem_fun(this, &tester_window::on_feedback));
		bot_chooser.set_sensitive(false);
		feedback.set_bot(bot);
	} else {
		bot_chooser.set_sensitive(true);
		feedback.set_bot(xbee_drive_bot::ptr());
		drive_chooser.set_sensitive(false);
		dribble_scale.set_sensitive(false);
		chicker_enabled.set_sensitive(false);
		chicker_kick.set_sensitive(false);
		chicker_chip.set_sensitive(false);
		assert(bot->refs() == 1);
		bot.reset();
	}
}

void tester_window::on_bot_alive() {
	drive_chooser.set_sensitive(true);
	dribble_scale.set_sensitive(true);
	chicker_enabled.set_sensitive(true);
	chicker_kick.set_sensitive(true);
	chicker_chip.set_sensitive(true);
	DPRINT("Bot alive.");
}

void tester_window::on_bot_dead() {
	DPRINT("Bot dead.");
}

void tester_window::on_bot_claim_failed_locked() {
	on_bot_claim_failed("This robot cannot be claimed because it is already claimed by another client.");
}

void tester_window::on_bot_claim_failed_resource() {
	on_bot_claim_failed("This robot cannot be claimed because there are insufficient available radio resources.");
}

void tester_window::on_bot_claim_failed(const Glib::ustring &message) {
	claim_bot_button.set_active(false);
	Gtk::MessageDialog md(*this, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	md.set_title("Thunderbots Tester");
	md.run();
}

int tester_window::key_snoop(Widget *, GdkEventKey *event) {
	if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Z || event->keyval == GDK_z)) {
		// Z letter scrams the system.
		drive_chooser.set_active_text("Halt");
		dribble_scale.set_value(0);
		chicker_enabled.set_active(false);
	} else if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_0)) {
		// Zero digit sets all controls to zero but does not scram things.
		if (drive_zeroable) {
			drive_zeroable->zero();
		}
		dribble_scale.set_value(0);
	}
	return 0;
}

void tester_window::drive_mode_changed() {
	// Remove any UI control.
	if (drive_widget) {
		drive_box.remove(*drive_widget);
		drive_widget = 0;
		drive_zeroable = 0;
	}

	// Zero out the drive levels.
	bot->drive_scram();

	// Create the new controls.
	const Glib::ustring &cur = drive_chooser.get_active_text();
	if (cur == "Halt") {
		// No controls, but need to scram the drive motors.
		if (bot) {
			bot->drive_scram();
		}
	} else if (cur == "Direct Drive") {
		// Use the direct drive controls.
		tester_control_direct_drive *drive = new tester_control_direct_drive(bot);
		drive = Gtk::manage(drive);
		drive_widget = drive;
		drive_zeroable = drive;
	} else if (cur == "Controlled Per-Motor Drive") {
		// Use the controlled per-motor drive controls.
		tester_control_controlled_permotor_drive *drive = Gtk::manage(new tester_control_controlled_permotor_drive(bot));
		drive_widget = drive;
		drive_zeroable = drive;
	} else if (cur == "Matrix Drive") {
		// Use the matrix drive controls.
		tester_control_matrix_drive *drive = Gtk::manage(new tester_control_matrix_drive(bot));
		drive_widget = drive;
		drive_zeroable = drive;
	}

	// Add the new controls to the UI.
	if (drive_widget) {
		drive_box.pack_start(*drive_widget);
		drive_widget->show_all();
	}
}

void tester_window::on_dribble_change() {
	if (bot) {
		bot->dribble(dribble_scale.get_value());
	}
}

void tester_window::on_chicker_enable_change() {
	if (bot) {
		bot->enable_chicker(chicker_enabled.get_active());
	}
}

void tester_window::on_chicker_kick() {
	if (bot) {
		bot->kick(static_cast<unsigned int>(chicker_power.get_value() + 0.1) / 32);
	}
}

void tester_window::on_chicker_chip() {
	if (bot) {
		bot->chip(static_cast<unsigned int>(chicker_power.get_value() + 0.1) / 32);
	}
}

void tester_window::on_feedback() {
	if (bot) {
		if (bot->feedback().flags & xbeepacket::FEEDBACK_FLAG_CHICKER_FAULT) {
			chicker_status.set_colour(1, 0, 0);
		} else if (bot->feedback().flags & xbeepacket::FEEDBACK_FLAG_CHICKER_READY) {
			chicker_status.set_colour(0, 1, 0);
		} else {
			chicker_status.set_colour(0, 0, 0);
		}
		bot->stamp();
	} else {
		chicker_status.set_colour(0, 0, 0);
	}
}

