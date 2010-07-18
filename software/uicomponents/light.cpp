#include "uicomponents/light.h"
#include <algorithm>

Light::Light() : r(0), g(0), b(0) {
	set_size_request(24, 24);
}

Light::Light(const Glib::ustring &label) : label(label), r(0), g(0), b(0) {
	set_size_request(24, 24);
}

void Light::set_colour(double r, double g, double b) {
	if (r != this->r || g != this->g || b != this->b) {
		this->r = r;
		this->g = g;
		this->b = b;
		Glib::RefPtr<Gdk::Window> win = get_window();
		if (win) {
			win->invalidate(false);
		}
	}
}

void Light::set_label(const Glib::ustring &l) {
	label = l;
	const Glib::RefPtr<Gdk::Window> win(get_window());
	if (win) {
		win->invalidate(false);
	}
}

bool Light::on_expose_event(GdkEventExpose *) {
	int width, height;
	get_window()->get_size(width, height);

	int mindim = std::min(width, height) - 2;

	Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();
	ctx->set_source_rgb(r, g, b);
	ctx->arc(width / 2.0, height / 2.0, mindim / 2.0, 0.0, 2 * M_PI);
	ctx->fill();
	ctx->set_source_rgb(0, 0, 0);
	ctx->arc(width / 2.0, height / 2.0, mindim / 2.0, 0.0, 2 * M_PI);
	ctx->stroke();

	ctx->set_source_rgb(0.0, 0.0, 0.0);
	const std::string &str(label);
	Cairo::TextExtents extents;
	ctx->get_text_extents(str, extents);
	const double x = width / 2.0 - extents.x_bearing - extents.width / 2.0;
	const double y = height / 2.0 - extents.y_bearing - extents.height / 2.0;
	ctx->move_to(x, y);
	ctx->show_text(str);

	// Done.
	return true;
}

