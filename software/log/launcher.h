#ifndef LOG_LAUNCHER_H
#define LOG_LAUNCHER_H

#include "util/fd.h"
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <glibmm/dispatcher.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/listviewtext.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/window.h>

/**
 * A window from which the user can select recorded logs and launch tools on them.
 */
class LogLauncher final : public Gtk::Window {
	public:
		/**
		 * Creates a new LogLauncher.
		 */
		explicit LogLauncher();

	private:
		Gtk::VBox vbox;
		Gtk::HBox hbox;
		Gtk::ScrolledWindow scroller;
		Gtk::ListViewText log_list;
		Gtk::VButtonBox vbb;
		Gtk::Button analyzer_button;
		Gtk::Button player_button;
		Gtk::Button rename_button;
		Gtk::Button delete_button;
		Gtk::Button export_button;
		Gtk::Button import_button;
		std::vector<std::string> files;
		std::vector<std::string> files_to_compress;
		std::vector<std::string>::const_iterator next_file_to_compress;
		Gtk::ProgressBar compress_progress_bar;
		std::vector<std::thread> compress_threads;
		std::vector<std::thread::id> compress_threads_done;
		std::mutex compress_threads_done_mutex;
		Glib::Dispatcher compress_dispatcher;
		FileDescriptor compress_lock_fd;
		bool exit_pending;

		void populate();
		void start_compressing();
		void compress_thread_proc(const std::string &filename);
		bool on_delete_event(GdkEventAny *);
		void on_log_list_selection_changed();
		void on_analyzer_clicked();
		void on_player_clicked();
		void on_rename_clicked();
		void on_delete_clicked();
		void on_export_clicked();
		void on_import_clicked();
};

#endif

