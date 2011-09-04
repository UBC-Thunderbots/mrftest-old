#ifndef AI_PARAM_PANEL_H
#define AI_PARAM_PANEL_H

#include <gtkmm.h>

class ParamPanel : public Gtk::VBox {
	public:
		ParamPanel();

	private:
		Gtk::ScrolledWindow scroller;
		Gtk::HButtonBox hbb;
		Gtk::Button load_button, clear_button, save_button;
};

#endif

