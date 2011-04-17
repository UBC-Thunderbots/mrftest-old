#ifndef LOG_LAUNCHER_H
#define LOG_LAUNCHER_H

#include <gtkmm.h>

/**
 * A window from which the user can select recorded logs and launch tools on them.
 */
class LogLauncher : public Gtk::Window {
	public:
		/**
		 * Creates a new LogLauncher.
		 */
		LogLauncher();

	private:
		Gtk::ListViewText log_list;
		Gtk::Button analyzer_button;
		Gtk::Button rename_button;
		Gtk::Button delete_button;

		void on_log_list_selection_changed();
		void on_analyzer_clicked();
		void on_rename_clicked();
		void on_delete_clicked();
};

#endif

