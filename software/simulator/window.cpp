#include "simulator/window.h"
#include "uicomponents/visualizer.h"
#include <gtkmm.h>

class simulator_window_impl : public virtual Gtk::Window {
	public:
		simulator_window_impl();

	protected:
		virtual bool on_delete_event(GdkEventAny *);

	private:
		Gtk::HBox hbox;
		Gtk::VBox vbox;

		Gtk::Frame engine_frame;

		Gtk::Frame playtype_frame;

		Gtk::Frame westteam_frame;

		Gtk::Frame eastteam_frame;

		Gtk::EventBox visualizer_box;
};

simulator_window_impl::simulator_window_impl() : engine_frame("Simulation Engine"), playtype_frame("Play Type"), westteam_frame("West Team"), eastteam_frame("East Team") {
	set_title("Thunderbots Simulator");
	vbox.pack_start(engine_frame);
	vbox.pack_start(playtype_frame);
	vbox.pack_start(westteam_frame);
	vbox.pack_start(eastteam_frame);
	hbox.pack_start(vbox);
	hbox.pack_start(visualizer_box);
	add(hbox);
	show_all();
}

bool simulator_window_impl::on_delete_event(GdkEventAny *) {
	Gtk::Main::quit();
	return true;
}

simulator_window::simulator_window() : impl(new simulator_window_impl) {
}

simulator_window::~simulator_window() {
	delete impl;
}

