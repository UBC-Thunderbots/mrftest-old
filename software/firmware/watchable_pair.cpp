#include "firmware/watchable_pair.h"

WatchablePair::WatchablePair(WatchableOperation &op1, WatchableOperation &op2, double weight) : op1(op1), op2(op2), weight(weight) {
	op1.signal_progress.connect(sigc::mem_fun(this, &WatchablePair::on_op1_progress));
	op2.signal_progress.connect(sigc::mem_fun(this, &WatchablePair::on_op2_progress));
	op1.signal_finished.connect(sigc::mem_fun(this, &WatchablePair::on_op1_finished));
	op2.signal_finished.connect(signal_finished.make_slot());
	op1.signal_error.connect(signal_error.make_slot());
	op2.signal_error.connect(signal_error.make_slot());
	status = op1.get_status();
}

void WatchablePair::start() {
	op1.start();
}

void WatchablePair::on_op1_progress(double progress) {
	status = op1.get_status();
	signal_progress.emit(progress * weight);
}

void WatchablePair::on_op2_progress(double progress) {
	status = op2.get_status();
	signal_progress.emit(progress * (1.0 - weight) + weight);
}

void WatchablePair::on_op1_finished() {
	status = op2.get_status();
	signal_progress.emit(0.5);
	op2.start();
}

