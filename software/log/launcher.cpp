#include "log/launcher.h"
#include "log/analyzer.h"
#include "util/exception.h"
#include <algorithm>
#include <cerrno>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
	void populate(Gtk::ListViewText &log_list) {
		log_list.clear_items();
		const std::string &parent_dir = Glib::get_user_data_dir();
		const std::string &tbots_dir = Glib::build_filename(parent_dir, "thunderbots");
		const std::string &logs_dir = Glib::build_filename(tbots_dir, "logs");
		Glib::Dir dir(logs_dir);
		std::vector<std::string> logs(dir.begin(), dir.end());
		std::sort(logs.begin(), logs.end());
		std::for_each(logs.begin(), logs.end(), sigc::compose(sigc::mem_fun(log_list, &Gtk::ListViewText::append_text), &Glib::filename_to_utf8));
	}
}

LogLauncher::LogLauncher() : log_list(1, false, Gtk::SELECTION_EXTENDED), analyzer_button("Analyzer"), rename_button("Rename"), delete_button("Delete") {
	set_title("Thunderbots Log Tools");
	set_size_request(400, 400);

	populate(log_list);

	Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox);

	Gtk::ScrolledWindow *scroller = Gtk::manage(new Gtk::ScrolledWindow);
	log_list.set_column_title(0, "Log File");
	scroller->add(log_list);
	hbox->pack_start(*scroller, Gtk::PACK_EXPAND_WIDGET);

	Gtk::VButtonBox *vbb = Gtk::manage(new Gtk::VButtonBox(Gtk::BUTTONBOX_SPREAD));
	vbb->pack_start(analyzer_button);
	vbb->pack_start(rename_button);
	vbb->pack_start(delete_button);
	hbox->pack_start(*vbb, Gtk::PACK_SHRINK);

	add(*hbox);

	log_list.get_selection()->signal_changed().connect(sigc::mem_fun(this, &LogLauncher::on_log_list_selection_changed));
	on_log_list_selection_changed();
	analyzer_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_analyzer_clicked));
	rename_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_rename_clicked));
	delete_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_delete_clicked));

	show_all();
}

LogLauncher::~LogLauncher() {
}

void LogLauncher::on_log_list_selection_changed() {
	int num = log_list.get_selection()->count_selected_rows();
	analyzer_button.set_sensitive(num == 1);
	rename_button.set_sensitive(num == 1);
	delete_button.set_sensitive(num >= 1);
}

void LogLauncher::on_analyzer_clicked() {
	new LogAnalyzer(*this, Glib::filename_from_utf8(log_list.get_text(log_list.get_selected()[0])));
}

void LogLauncher::on_rename_clicked() {
	Gtk::Dialog dlg("Thunderbots Log Tools - Rename Log", *this, true);
	Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox);
	hbox->pack_start(*Gtk::manage(new Gtk::Label("Enter new name for log:")), Gtk::PACK_SHRINK);
	Gtk::Entry new_name_entry;
	new_name_entry.set_activates_default();
	new_name_entry.set_width_chars(30);
	new_name_entry.set_text(log_list.get_text(log_list.get_selected()[0]));
	hbox->pack_start(new_name_entry, Gtk::PACK_EXPAND_WIDGET);
	hbox->show_all();
	dlg.get_vbox()->pack_start(*hbox, Gtk::PACK_SHRINK);
	dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dlg.set_default_response(Gtk::RESPONSE_OK);
	int resp = dlg.run();
	if (resp == Gtk::RESPONSE_OK) {
		const std::string &old_name = Glib::filename_from_utf8(log_list.get_text(log_list.get_selected()[0]));
		const std::string &new_name = Glib::filename_from_utf8(new_name_entry.get_text());
		if (new_name != old_name) {
			const std::string &parent_dir = Glib::get_user_data_dir();
			const std::string &tbots_dir = Glib::build_filename(parent_dir, "thunderbots");
			const std::string &logs_dir = Glib::build_filename(tbots_dir, "logs");
			const std::string &old_path = Glib::build_filename(logs_dir, old_name);
			const std::string &new_path = Glib::build_filename(logs_dir, new_name);
			try {
				if (link(old_path.c_str(), new_path.c_str()) < 0) {
					throw SystemError("link", errno);
				}
				if (unlink(old_path.c_str()) < 0) {
					throw SystemError("unlink", errno);
				}
			} catch (const SystemError &exp) {
				Gtk::MessageDialog md(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
				md.run();
			}
			populate(log_list);
		}
	}
}

void LogLauncher::on_delete_clicked() {
	int num = log_list.get_selection()->count_selected_rows();
	Gtk::MessageDialog md(*this, Glib::ustring::compose("Are you sure you wish to delete %1 %2 log%3?", num == 1 ? "this" : "these", num, num == 1 ? "" : "s"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
	int resp = md.run();
	if (resp == Gtk::RESPONSE_YES) {
		const std::string &parent_dir = Glib::get_user_data_dir();
		const std::string &tbots_dir = Glib::build_filename(parent_dir, "thunderbots");
		const std::string &logs_dir = Glib::build_filename(tbots_dir, "logs");
		const Gtk::TreeSelection::ListHandle_Path &selected = log_list.get_selection()->get_selected_rows();
		for (Gtk::TreeSelection::ListHandle_Path::const_iterator i = selected.begin(), iend = selected.end(); i != iend; ++i) {
			const std::string &filename = Glib::build_filename(logs_dir, Glib::filename_from_utf8(log_list.get_text((*i)[0])));
			try {
				if (unlink(filename.c_str()) < 0) {
					throw SystemError("unlink", errno);
				}
			} catch (const SystemError &exp) {
				Gtk::MessageDialog md2(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
				md2.run();
			}
		}
		populate(log_list);
	}
}

