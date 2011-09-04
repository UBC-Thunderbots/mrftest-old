#ifndef LOG_ANALYZER_H
#define LOG_ANALYZER_H

#include <gtkmm.h>
#include <memory>
#include <string>

class LogAnalyzer : public Gtk::Window {
	public:
		/**
		 * \brief Creates a new LogAnalyzer.
		 *
		 * \param[in] parent the parent window.
		 *
		 * \param[in] pathname the path to the log file to analyze.
		 */
		LogAnalyzer(Gtk::Window &parent, const std::string &pathname);

	private:
		class Impl;

		std::unique_ptr<Impl> impl;
		Gtk::VBox vbox;
		Gtk::HPaned hpaned;
		bool pane_fixed;
		Gtk::TreeView packets_list_view;
		Gtk::TreeView packet_decoded_tree;
		Gtk::Button to_tsv_button;

		bool on_delete_event(GdkEventAny *);
		void on_size_allocate(Gtk::Allocation &alloc);
		void on_packets_list_view_selection_changed();
		void on_to_tsv_clicked();
};

#endif

