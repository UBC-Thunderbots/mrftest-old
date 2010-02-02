#ifndef LOG_TOOLS_MERGER_H
#define LOG_TOOLS_MERGER_H

#include "util/noncopyable.h"
#include <string>
#include <vector>
#include <gtkmm.h>

//
// Provides the ability to merge multiple log files into a single file.
//
class log_merger : public Gtk::Window, public noncopyable {
	public:
		//
		// Constructs a new log_merger.
		//
		log_merger(const std::vector<std::string> &logs, Gtk::Window &parent);

		//
		// The signal emitted when a merge finishes.
		//
		sigc::signal<void> &signal_merge_complete() {
			return sig_merge_complete;
		}

	protected:
		bool on_delete_event(GdkEventAny *);

	private:
		sigc::signal<void> sig_merge_complete;

		std::vector<std::string> logs;

		Gtk::VBox vbox;

		Gtk::HBox input_box;
		Gtk::ScrolledWindow input_list_scroll;
		Gtk::ListViewText input_list;
		Gtk::VButtonBox input_updown_box;
		Gtk::Button input_move_up_button;
		Gtk::Button input_move_down_button;

		Gtk::Button merge_button;

		void update_sensitivity();
		void move_up_clicked();
		void move_down_clicked();
		void merge_clicked();
};

#endif

