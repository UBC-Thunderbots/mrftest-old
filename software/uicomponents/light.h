#ifndef UICOMPONENTS_LIGHT_H
#define UICOMPONENTS_LIGHT_H

#include <gtkmm.h>

//
// A generic "light" that can be made to glow any colour.
//
class light : public Gtk::DrawingArea {
	public:
		light();
		light(const Glib::ustring &label);
		void set_colour(double r, double g, double b);
		void set_label(const Glib::ustring &label);

	private:
		Glib::ustring label;
		double r, g, b;

		bool on_expose_event(GdkEventExpose *);
};

#endif

