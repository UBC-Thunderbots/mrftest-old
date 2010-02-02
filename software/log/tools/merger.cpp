#include "log/reader/reader.h"
#include "log/tools/merger.h"
#include "log/writer/writer.h"
#include <algorithm>



log_merger::log_merger(const std::vector<std::string> &logs, Gtk::Window &parent) : logs(logs), input_list(1), input_updown_box(Gtk::BUTTONBOX_SPREAD), input_move_up_button(Gtk::Stock::GO_UP), input_move_down_button(Gtk::Stock::GO_DOWN), merge_button(Gtk::Stock::EXECUTE) {
	set_title("Log Merger");
	set_transient_for(parent);
	set_modal();
	for (std::vector<std::string>::const_iterator i = logs.begin(), iend = logs.end(); i != iend; ++i) {
		input_list.append_text(Glib::filename_display_name(*i));
	}
	input_list.set_headers_visible(false);
	input_list.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &log_merger::update_sensitivity));
	input_list_scroll.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	update_sensitivity();
	input_move_up_button.signal_clicked().connect(sigc::mem_fun(*this, &log_merger::move_up_clicked));
	input_move_down_button.signal_clicked().connect(sigc::mem_fun(*this, &log_merger::move_down_clicked));
	merge_button.signal_clicked().connect(sigc::mem_fun(*this, &log_merger::merge_clicked));

	input_list_scroll.add(input_list);
	input_box.pack_start(input_list_scroll, true, true);
	input_updown_box.pack_start(input_move_up_button);
	input_updown_box.pack_start(input_move_down_button);
	input_box.pack_start(input_updown_box, false, true);
	vbox.pack_start(input_box, true, true);
	vbox.pack_start(merge_button, false, true);
	add(vbox);
	show_all();
}



bool log_merger::on_delete_event(GdkEventAny *) {
	delete this;
	return true;
}



void log_merger::update_sensitivity() {
	const Gtk::ListViewText::SelectionList &sel = input_list.get_selected();
	if (sel.size() == 1) {
		input_move_up_button.set_sensitive(sel[0] > 0);
		input_move_down_button.set_sensitive(static_cast<unsigned int>(sel[0] + 1) < logs.size());
	} else {
		input_move_up_button.set_sensitive(false);
		input_move_down_button.set_sensitive(false);
	}
}



void log_merger::move_up_clicked() {
	int idx = input_list.get_selected()[0];
	std::swap(logs[idx], logs[idx - 1]);
	input_list.set_text(idx - 1, Glib::filename_display_name(logs[idx - 1]));
	input_list.set_text(idx, Glib::filename_display_name(logs[idx]));
	input_list.get_selection()->unselect_all();
	Gtk::TreePath path;
	path.push_back(idx - 1);
	input_list.get_selection()->select(path);
}



void log_merger::move_down_clicked() {
	int idx = input_list.get_selected()[0];
	std::swap(logs[idx], logs[idx + 1]);
	input_list.set_text(idx + 1, Glib::filename_display_name(logs[idx + 1]));
	input_list.set_text(idx, Glib::filename_display_name(logs[idx]));
	input_list.get_selection()->unselect_all();
	Gtk::TreePath path;
	path.push_back(idx + 1);
	input_list.get_selection()->select(path);
}



void log_merger::merge_clicked() {
	log_writer::ptr writer(log_writer::create());
	for (std::vector<std::string>::const_iterator i = logs.begin(), iend = logs.end(); i != iend; ++i) {
		log_reader::ptr reader(log_reader::create(*i));
		writer->write_frame(reader->get_field(), reader->get_ball(), reader->get_west_team(), reader->get_east_team());
		while (reader->tell() + 1 < reader->size()) {
			reader->next_frame();
			writer->write_frame(reader->get_field(), reader->get_ball(), reader->get_west_team(), reader->get_east_team());
		}
	}
	sig_merge_complete.emit();
	Gtk::MessageDialog(*this, "Merge complete.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true).run();
	delete this;
}

