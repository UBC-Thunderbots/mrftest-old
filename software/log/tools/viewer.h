#ifndef LOG_TOOLS_VIEWER_H
#define LOG_TOOLS_VIEWER_H

#include "log/reader/reader.h"
#include "uicomponents/visualizer.h"
#include "util/clocksource_timerfd.h"
#include "util/noncopyable.h"
#include <string>
#include <gtkmm.h>

//
// A window that plays back a log file.
//
class log_viewer : public Gtk::Window, public noncopyable {
	public:
		log_viewer(const std::string &name);

	protected:
		bool on_delete_event(GdkEventAny *);

	private:
		clocksource_timerfd clk;
		log_reader::ptr reader;

		Gtk::VBox vbox;

		visualizer vis;

		Gtk::HBox control_box;
		Gtk::ToggleButton play_button;
		Gtk::HScale frame_scale;

		void on_tick();
		void on_play_toggled();
		void on_frame_scale_moved();
};

#endif

