#ifndef LOG_ANALYZER_H
#define LOG_ANALYZER_H

#include "util/scoped_ptr.h"
#include <gtkmm.h>
#include <string>

class LogAnalyzer : public Gtk::Window {
	public:
		/**
		 * Creates a new LogAnalyzer.
		 *
		 * \param[in] parent the parent window.
		 *
		 * \param[in] filename the name of the log file to analyze.
		 */
		LogAnalyzer(Gtk::Window &parent, const std::string &filename);

	private:
		class Impl;

		ScopedPtr<Impl> impl;
		Gtk::VBox vbox;
		Gtk::HPaned hpaned;
		Gtk::VPaned vpaned;
		bool panes_fixed;
		Gtk::TreeView packets_list_view;
		Gtk::TextView packet_raw_entry;
		Gtk::TreeView packet_decoded_tree;
		Gtk::Button to_tsv_button;

		~LogAnalyzer();
		bool on_delete_event(GdkEventAny *);
		void on_size_allocate(Gtk::Allocation &alloc);
		void on_packets_list_view_selection_changed();
		void on_to_tsv_clicked();
};

#endif

