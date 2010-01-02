#include "geom/angle.h"
#include "uicomponents/light.h"
#include <algorithm>



light::light() : r(0), g(0), b(0) {
	set_size_request(24, 24);
}

void light::set_colour(double r, double g, double b) {
	this->r = r;
	this->g = g;
	this->b = b;
}

bool light::on_expose_event(GdkEventExpose *) {
	int width, height;
	get_window()->get_size(width, height);

	int mindim = std::min(width, height) - 2;

	Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();
	ctx->set_source_rgb(r, g, b);
	ctx->arc(width / 2.0, height / 2.0, mindim / 2.0, 0.0, 2 * PI);
	ctx->fill();
	ctx->set_source_rgb(0, 0, 0);
	ctx->arc(width / 2.0, height / 2.0, mindim / 2.0, 0.0, 2 * PI);
	ctx->stroke();

	// Done.
	return true;
}

