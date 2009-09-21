#include "uicomponents/visualizer.h"
#include <algorithm>
#include <cairomm/cairomm.h>
#include <iostream>



visualizer::visualizer(const field::ptr field, const ball::ptr ball, const team::ptr west_team, const team::ptr east_team) : the_field(field), the_ball(ball), west_team(west_team), east_team(east_team) {
}

void visualizer::update() {
	int width, height;
	get_window()->get_size(width, height);
	Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();

	// Fill the background with field-green.
	ctx->set_source_rgb(0.0, 0.75, 0.0);
	ctx->move_to(0.0, 0.0);
	ctx->line_to(width, 0.0);
	ctx->line_to(width, height);
	ctx->line_to(0.0, height);
	ctx->fill();

	// Establish a transformation from global world coordinates to screen coordinates.
	ctx->translate(width / 2.0, height / 2.0);
	double xscale = width / (the_field->length() * 1.2);
	double yscale = height / (the_field->width() * 1.2);
	double scale = std::min(xscale, yscale);
	ctx->scale(scale, -scale);
	ctx->set_line_width(2.0 / scale);

	// Draw the rectangular outline.
	ctx->set_source_rgb(1.0, 1.0, 1.0);
	ctx->move_to(-the_field->length() / 2.0, -the_field->width() / 2.0);
	ctx->line_to( the_field->length() / 2.0, -the_field->width() / 2.0);
	ctx->line_to( the_field->length() / 2.0,  the_field->width() / 2.0);
	ctx->line_to(-the_field->length() / 2.0,  the_field->width() / 2.0);
	ctx->line_to(-the_field->length() / 2.0, -the_field->width() / 2.0);
	ctx->stroke();

	// Draw the centre line.
	ctx->move_to(0.0, -the_field->width() / 2.0);
	ctx->line_to(0.0,  the_field->width() / 2.0);
	ctx->stroke();

	// Draw the centre circle.
	ctx->arc(0.0, 0.0, the_field->centre_circle_radius(), 0.0, 2 * PI);
	ctx->stroke();

	// Draw the west defense area.
	ctx->arc(-the_field->length() / 2.0, -the_field->defense_area_stretch() / 2.0, the_field->defense_area_radius(), -PI / 2.0, 0.0);
	ctx->move_to(-the_field->length() / 2.0 + the_field->defense_area_radius(), -the_field->defense_area_stretch() / 2.0);
	ctx->line_to(-the_field->length() / 2.0 + the_field->defense_area_radius(),  the_field->defense_area_stretch() / 2.0);
	ctx->arc(-the_field->length() / 2.0,  the_field->defense_area_stretch() / 2.0, the_field->defense_area_radius(), 0.0,  PI / 2.0);
	ctx->stroke();

	// Draw the east defense area.
	ctx->arc( the_field->length() / 2.0, -the_field->defense_area_stretch() / 2.0, the_field->defense_area_radius(), PI, 3.0 * PI / 2.0);
	ctx->move_to( the_field->length() / 2.0 - the_field->defense_area_radius(), -the_field->defense_area_stretch() / 2.0);
	ctx->line_to( the_field->length() / 2.0 - the_field->defense_area_radius(),  the_field->defense_area_stretch() / 2.0);
	ctx->arc_negative( the_field->length() / 2.0,  the_field->defense_area_stretch() / 2.0, the_field->defense_area_radius(), PI, PI / 2.0);
	ctx->stroke();

	// Draw the ball.
	ctx->set_source_rgb(1.0, 0.5, 0.0);
	ctx->arc(the_ball->position().real(), the_ball->position().imag(), 0.03, 0.0, 2.0 * PI);
	ctx->fill();

	// Draw the players.
	const team::ptr teams[2] = {west_team, east_team};
	for (unsigned int i = 0; i < 2; i++) {
		if (teams[i]->yellow())
			ctx->set_source_rgb(1.0, 1.0, 0.0);
		else
			ctx->set_source_rgb(0.0, 1.0, 1.0);

		for (unsigned int j = 0; j < teams[i]->size(); j++) {
			robot::ptr bot = teams[i]->get_robot(j);
			ctx->arc(bot->position().real(), bot->position().imag(), 0.09, bot->orientation() + PI / 4.0, bot->orientation() - PI / 4.0);
			ctx->fill();
		}
	}

	// Draw the text on the players. Note that Y-axis negation is done manually here
	// because otherwise the text itself would be upside-down.
	ctx->set_source_rgb(0.0, 0.0, 0.0);
	ctx->set_font_size(0.09);
	ctx->scale(1.0, -1.0);
	for (unsigned int i = 0; i < 2; i++) {
		for (unsigned int j = 0; j < teams[i]->size(); j++) {
			robot::ptr bot = teams[i]->get_robot(j);
			const Glib::ustring &ustr = Glib::ustring::compose("%1", bot->id());
			const std::string &str = ustr;
			Cairo::TextExtents extents;
			ctx->get_text_extents(str, extents);
			const double x = bot->position().real() - extents.x_bearing - extents.width / 2.0;
			const double y = -bot->position().imag() - extents.y_bearing - extents.height / 2.0;
			ctx->move_to(x, y);
			ctx->show_text(str);
		}
	}
}

