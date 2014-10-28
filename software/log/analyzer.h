#ifndef LOG_ANALYZER_H
#define LOG_ANALYZER_H

#include <memory>
#include <string>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/frame.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>

class LogAnalyzer final : public Gtk::Window {
	public:
		/**
		 * \brief Creates a new LogAnalyzer.
		 *
		 * \param[in] parent the parent window.
		 *
		 * \param[in] pathname the path to the log file to analyze.
		 */
		explicit LogAnalyzer(Gtk::Window &parent, const std::string &pathname);

	private:
		class Impl;

		std::unique_ptr<Impl> impl;
		Gtk::VBox vbox;
		Gtk::HPaned hpaned;
		bool pane_fixed;
		Gtk::Frame packets_list_frame;
		Gtk::ScrolledWindow packets_list_scroller;
		Gtk::TreeView packets_list_view;
		Gtk::Frame packet_decoded_frame;
		Gtk::ScrolledWindow packet_decoded_scroller;
		Gtk::TreeView packet_decoded_tree;
		Gtk::Button to_tsv_button;

		bool on_delete_event(GdkEventAny *);
		void on_size_allocate(Gtk::Allocation &alloc);
		void on_packets_list_view_selection_changed();
		void on_to_tsv_clicked();
};

#endif

