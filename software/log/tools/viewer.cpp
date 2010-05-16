#include "log/tools/viewer.h"
#include "util/timestep.h"
#include <stdint.h>



log_viewer::log_viewer(const std::string &name) : clk(UINT64_C(1000000000) / TIMESTEPS_PER_SECOND), reader(log_reader::create(name)), vbox(false, 10), vis(reader->get_field(), reader->get_ball(), reader->get_west_team(), reader->get_east_team(), false), control_box(false, 10), play_button(Gtk::Stock::MEDIA_PLAY), frame_scale(0, reader->size() - 1, 1) {
	frame_scale.get_adjustment()->set_page_size(0);
	clk.signal_tick().connect(sigc::mem_fun(this, &log_viewer::on_tick));
	play_button.signal_toggled().connect(sigc::mem_fun(this, &log_viewer::on_play_toggled));
	frame_scale.signal_value_changed().connect(sigc::mem_fun(this, &log_viewer::on_frame_scale_moved));

	vbox.pack_start(vis, true, true);

	control_box.pack_start(play_button, false, true);
	control_box.pack_start(frame_scale, true, true);
	vbox.pack_start(control_box, false, true);

	add(vbox);
	show_all();
}



bool log_viewer::on_delete_event(GdkEventAny *) {
	delete this;
	return true;
}



void log_viewer::on_tick() {
	uint64_t cur_frame = frame_scale.get_value();
	if (cur_frame < reader->size() - 1) {
		frame_scale.set_value(cur_frame + 1);
	} else {
		play_button.set_active(false);
	}
}



void log_viewer::on_play_toggled() {
	if (play_button.get_active()) {
		clk.start();
	} else {
		clk.stop();
	}
}



void log_viewer::on_frame_scale_moved() {
	uint64_t frame = frame_scale.get_value();

	if (reader->tell() == frame) {
		// Do nothing.
	} else if (reader->tell() + 1 == frame) {
		reader->next_frame();
	} else {
		reader->seek(frame);
	}
	vis.update();

	if (frame < reader->size() - 1) {
		play_button.set_sensitive(true);
	} else {
		play_button.set_sensitive(false);
		play_button.set_active(false);
	}
}

