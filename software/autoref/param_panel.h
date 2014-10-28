#ifndef AI_PARAM_PANEL_H
#define AI_PARAM_PANEL_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/scrolledwindow.h>

class ParamPanel final : public Gtk::VBox {
	public:
		explicit ParamPanel();

	private:
		Gtk::ScrolledWindow scroller;
		Gtk::HButtonBox hbb;
		Gtk::Button load_button, clear_button, save_button;
};

#endif

