#ifndef UICOMPONENTS_LIGHT_H
#define UICOMPONENTS_LIGHT_H

#include <gtkmm.h>

/**
 * A generic "light" that can be made to glow any colour.
 */
class Light : public Gtk::DrawingArea {
	public:
		/**
		 * Constructs a new Light.
		 */
		Light();

		/**
		 * Constructs a new Light.
		 *
		 * \param[in] label the label to attach to the light.
		 */
		Light(const Glib::ustring &label);

		/**
		 * Sets the colour of the light.
		 *
		 * \param[in] r the red component, from 0 to 1.
		 *
		 * \param[in] g the green component, from 0 to 1.
		 *
		 * \param[in] b the blue component, from 0 to 1.
		 */
		void set_colour(double r, double g, double b);

		/**
		 * Sets the label on the light.
		 *
		 * \param[in] label the new label.
		 */
		void set_label(const Glib::ustring &label);

	private:
		Glib::ustring label;
		double r, g, b;

		bool on_expose_event(GdkEventExpose *);
};

#endif

