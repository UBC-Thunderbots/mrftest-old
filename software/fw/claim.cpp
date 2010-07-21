#include "fw/claim.h"

Claim::Claim(XBeeRawBot::ptr bot) : bot(bot), started(false), already_alive(false), already_failed(false) {
	bot->signal_alive.connect(sigc::mem_fun(this, &Claim::on_alive));
	bot->signal_claim_failed.connect(sigc::mem_fun(this, &Claim::on_failed));
	status = "Claiming";
}

void Claim::start() {
	if (already_failed) {
		signal_error.emit("The robot could not be claimed. Check that no other process is using the robot.");
	} else if (already_alive) {
		signal_finished.emit();
	} else {
		started = true;
	}
}

void Claim::on_alive() {
	if (started) {
		signal_finished.emit();
		started = false;
	} else {
		already_alive = true;
	}
}

void Claim::on_failed() {
	if (started) {
		signal_error.emit("The robot could not be claimed. Check that no other process is using the robot.");
		started = false;
	} else {
		already_failed = true;
	}
}

