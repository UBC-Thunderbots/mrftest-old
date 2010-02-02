#include "log/reader/reader.h"
#include "log/tools/merger.h"
#include "log/tools/tool_launcher.h"
#include "log/tools/viewer.h"
#include <gtkmm.h>
#include <unistd.h>



namespace {
	class log_chooser : public Gtk::ListViewText, public noncopyable {
		public:
			log_chooser() : ListViewText(1, false, Gtk::SELECTION_MULTIPLE) {
				set_headers_visible(false);
				refresh();
			}

			std::vector<std::string> get_selected_logs() {
				const std::vector<int> &rows = get_selected();
				std::vector<std::string> v;
				for (std::vector<int>::const_iterator i = rows.begin(), iend = rows.end(); i != iend; ++i) {
					v.push_back(logs[*i]);
				}
				return v;
			}

			void refresh() {
				clear_items();
				logs = log_reader::all_logs();
				for (std::vector<std::string>::const_iterator i = logs.begin(), iend = logs.end(); i != iend; ++i) {
					append_text(Glib::filename_display_name(*i));
				}
			}

		private:
			std::vector<std::string> logs;
	};
}



class log_tool_launcher_impl : public Gtk::Window, public noncopyable {
	public:
		log_tool_launcher_impl() : box(false, 10), button_box(Gtk::BUTTONBOX_SPREAD), view_button("Play"), rename_button("Rename"), delete_button("Delete"), merge_button("Merge") {
			set_title("Log Tools");
			chooser.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::update_sensitivity));
			chooser_scroll.add(chooser);
			chooser_scroll.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
			box.pack_start(chooser_scroll, true, true);

			button_box.pack_start(view_button);
			button_box.pack_start(rename_button);
			button_box.pack_start(delete_button);
			button_box.pack_start(merge_button);
			box.pack_start(button_box, false, true);

			add(box);
			show_all();

			update_sensitivity();
			view_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_view_clicked));
			rename_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_rename_clicked));
			delete_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_delete_clicked));
			merge_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_merge_clicked));
		}

	protected:
		bool on_delete_event(GdkEventAny *) {
			Gtk::Main::quit();
			return true;
		}

	private:
		Gtk::VBox box;

		Gtk::ScrolledWindow chooser_scroll;
		log_chooser chooser;

		Gtk::HButtonBox button_box;
		Gtk::Button view_button, rename_button, delete_button, merge_button;

		void update_sensitivity() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			if (sel.size() == 0) {
				view_button.set_sensitive(false);
				rename_button.set_sensitive(false);
				delete_button.set_sensitive(false);
				merge_button.set_sensitive(false);
			} else if (sel.size() == 1) {
				view_button.set_sensitive(true);
				rename_button.set_sensitive(true);
				delete_button.set_sensitive(true);
				merge_button.set_sensitive(false);
			} else {
				view_button.set_sensitive(false);
				rename_button.set_sensitive(false);
				delete_button.set_sensitive(true);
				merge_button.set_sensitive(true);
			}
		}

		void on_view_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			const std::string &name = sel[0];

			new log_viewer(name);
		}

		void on_rename_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			const std::string &old_name = sel[0];

			Gtk::Dialog dlg("Rename Log", *this, true);
			Gtk::Label lbl("Enter the new name:");
			lbl.show();
			dlg.get_vbox()->pack_start(lbl);
			Gtk::Entry entry;
			entry.set_text(Glib::filename_to_utf8(old_name));
			entry.set_activates_default(true);
			entry.show();
			dlg.get_vbox()->pack_start(entry);
			dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
			dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			dlg.set_default_response(Gtk::RESPONSE_OK);
			if (dlg.run() == Gtk::RESPONSE_OK) {
				const std::string &new_name = Glib::filename_from_utf8(entry.get_text());
				if (!new_name.empty()) {
					const std::string &dir = Glib::get_user_data_dir() + "/thunderbots/";
					if (rename((dir + old_name + ".log").c_str(), (dir + new_name + ".log").c_str()) < 0) {
						throw std::runtime_error("Cannot rename log file!");
					}
					if (rename((dir + old_name + ".idx").c_str(), (dir + new_name + ".idx").c_str()) < 0) {
						throw std::runtime_error("Cannot rename index file!");
					}
				}
				chooser.refresh();
			}
		}

		void on_delete_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			const Glib::ustring &msg = Glib::ustring::compose("Are you sure you want to delete %1 log%2?", sel.size(), sel.size() == 1 ? "" : "s");
			Gtk::MessageDialog dlg(*this, msg, false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
			dlg.set_title("Delete Logs");
			int rc = dlg.run();
			if (rc == Gtk::RESPONSE_YES) {
				for (std::vector<std::string>::const_iterator i = sel.begin(), iend = sel.end(); i != iend; ++i) {
					const std::string &base = Glib::get_user_data_dir() + "/thunderbots/" + *i;
					const std::string &logfile = base + ".log";
					const std::string &idxfile = base + ".idx";
					unlink(logfile.c_str());
					unlink(idxfile.c_str());
				}
				chooser.refresh();
			}
		}

		void on_merge_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			log_merger *m = new log_merger(sel, *this);
			m->signal_merge_complete().connect(sigc::mem_fun(chooser, &log_chooser::refresh));
		}
};



log_tool_launcher::log_tool_launcher() : impl(new log_tool_launcher_impl) {
}

log_tool_launcher::~log_tool_launcher() {
	delete impl;
}

