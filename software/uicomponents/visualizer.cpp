#include "uicomponents/visualizer.h"
#include <algorithm>
#include <cairomm/cairomm.h>
#include <iostream>



visualizer::visualizer(const field &field, const ball &ball, const team::ptr west_team, const team::ptr east_team) : the_field(field), the_ball(ball), west_team(west_team), east_team(east_team) {
}

void visualizer::update() {
	int width, height;
	get_window()->get_size(width, height);
	Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();

	ctx->set_source_rgb(0.0, 1.0, 0.0);
	ctx->move_to(0.0, 0.0);
	ctx->line_to(width, 0.0);
	ctx->line_to(width, height);
	ctx->line_to(0.0, height);
	ctx->fill();

	ctx->translate(width / 2.0, height / 2.0);
	double xscale = width / (the_field.length() * 1.2);
	double yscale = height / (the_field.width() * 1.2);
	double scale = std::min(xscale, yscale);
	ctx->scale(scale, scale);

	ctx->set_line_width(2.0 / scale);

	ctx->set_source_rgb(1.0, 1.0, 1.0);
	ctx->move_to(-the_field.length() / 2.0, -the_field.width() / 2.0);
	ctx->line_to( the_field.length() / 2.0, -the_field.width() / 2.0);
	ctx->line_to( the_field.length() / 2.0,  the_field.width() / 2.0);
	ctx->line_to(-the_field.length() / 2.0,  the_field.width() / 2.0);
	ctx->line_to(-the_field.length() / 2.0, -the_field.width() / 2.0);
	ctx->stroke();

	ctx->move_to(0.0, -the_field.width() / 2.0);
	ctx->line_to(0.0,  the_field.width() / 2.0);
	ctx->stroke();

	ctx->arc(0.0, 0.0, the_field.centre_circle_radius(), 0.0, 2 * PI);
	ctx->stroke();

	ctx->arc(-the_field.length() / 2.0, -the_field.defense_area_stretch() / 2.0, the_field.defense_area_radius(), -PI / 2.0, 0.0);
	ctx->move_to(-the_field.length() / 2.0 + the_field.defense_area_radius(), -the_field.defense_area_stretch() / 2.0);
	ctx->line_to(-the_field.length() / 2.0 + the_field.defense_area_radius(),  the_field.defense_area_stretch() / 2.0);
	ctx->arc(-the_field.length() / 2.0,  the_field.defense_area_stretch() / 2.0, the_field.defense_area_radius(), 0.0,  PI / 2.0);
	ctx->stroke();

	ctx->arc( the_field.length() / 2.0, -the_field.defense_area_stretch() / 2.0, the_field.defense_area_radius(), PI, 3.0 * PI / 2.0);
	ctx->move_to( the_field.length() / 2.0 - the_field.defense_area_radius(), -the_field.defense_area_stretch() / 2.0);
	ctx->line_to( the_field.length() / 2.0 - the_field.defense_area_radius(),  the_field.defense_area_stretch() / 2.0);
	ctx->arc_negative( the_field.length() / 2.0,  the_field.defense_area_stretch() / 2.0, the_field.defense_area_radius(), PI, PI / 2.0);
	ctx->stroke();

	ctx->set_source_rgb(1.0, 0.5, 0.0);
	ctx->arc(the_ball.position().real(), the_ball.position().imag(), 0.03, 0.0, 2.0 * PI);
	ctx->fill();
}

