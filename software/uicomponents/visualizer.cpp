// Set nonzero to draw velocity vectors.
#define DRAW_VELOCITY 0
// Set to the multiplier for visualizing velocity vector lengths.
#define DRAW_VELOCITY_SCALE 1.0

#include "uicomponents/visualizer.h"

visualizer::visualizer(const field::ptr field, const ball::ptr ball, const team::ptr west_team, const team::ptr east_team, bool draggable) : draggable(draggable), scale(1), xtranslate(0), ytranslate(0), the_field(field), the_ball(ball), west_team(west_team), east_team(east_team) {
	set_size_request(600, 600);
	add_events(Gdk::POINTER_MOTION_MASK);
	add_events(Gdk::BUTTON_PRESS_MASK);
	add_events(Gdk::BUTTON_RELEASE_MASK);
	add_events(Gdk::ENTER_NOTIFY_MASK);
	add_events(Gdk::LEAVE_NOTIFY_MASK);
	west_team->signal_robot_added().connect(sigc::mem_fun(this, &visualizer::update));
	west_team->signal_robot_removed().connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &visualizer::update))));
	east_team->signal_robot_added().connect(sigc::mem_fun(this, &visualizer::update));
	east_team->signal_robot_removed().connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &visualizer::update))));
}

bool visualizer::on_expose_event(GdkEventExpose *) {
	int width, height;
	get_window()->get_size(width, height);

	Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();
	ctx->set_font_size(dtog(0.09));

	// Fill the background with field-green.
	ctx->set_source_rgb(0.0, 0.75, 0.0);
	ctx->move_to(0.0, 0.0);
	ctx->line_to(width, 0.0);
	ctx->line_to(width, height);
	ctx->line_to(0.0, height);
	ctx->fill();

	// Draw the outline of the referee area.
	ctx->set_source_rgb(0.0, 0.0, 0.0);
	ctx->move_to(xtog(-the_field->total_length() / 2.0), ytog(-the_field->total_width() / 2.0));
	ctx->line_to(xtog( the_field->total_length() / 2.0), ytog(-the_field->total_width() / 2.0));
	ctx->line_to(xtog( the_field->total_length() / 2.0), ytog( the_field->total_width() / 2.0));
	ctx->line_to(xtog(-the_field->total_length() / 2.0), ytog( the_field->total_width() / 2.0));
	ctx->line_to(xtog(-the_field->total_length() / 2.0), ytog(-the_field->total_width() / 2.0));
	ctx->stroke();

	// Draw the rectangular outline.
	ctx->set_source_rgb(1.0, 1.0, 1.0);
	ctx->move_to(xtog(-the_field->length() / 2.0), ytog(-the_field->width() / 2.0));
	ctx->line_to(xtog( the_field->length() / 2.0), ytog(-the_field->width() / 2.0));
	ctx->line_to(xtog( the_field->length() / 2.0), ytog( the_field->width() / 2.0));
	ctx->line_to(xtog(-the_field->length() / 2.0), ytog( the_field->width() / 2.0));
	ctx->line_to(xtog(-the_field->length() / 2.0), ytog(-the_field->width() / 2.0));
	ctx->stroke();

	// Draw the centre line.
	ctx->move_to(xtog(0.0), ytog(-the_field->width() / 2.0));
	ctx->line_to(xtog(0.0), ytog( the_field->width() / 2.0));
	ctx->stroke();

	// Draw the centre circle.
	ctx->arc(xtog(0.0), ytog(0.0), dtog(the_field->centre_circle_radius()), 0.0, 2 * PI);
	ctx->stroke();

	// Draw the west defense area.
	ctx->arc(xtog(-the_field->length() / 2.0), ytog( the_field->defense_area_stretch() / 2.0), dtog(the_field->defense_area_radius()), 3 * PI / 2, 2 * PI);
	ctx->line_to(xtog(-the_field->length() / 2.0 + the_field->defense_area_radius()), ytog( the_field->defense_area_stretch() / 2.0));
	ctx->arc(xtog(-the_field->length() / 2.0), ytog(-the_field->defense_area_stretch() / 2.0), dtog(the_field->defense_area_radius()), 0, PI / 2);
	ctx->stroke();

	// Draw the east defense area.
	ctx->arc_negative(xtog( the_field->length() / 2.0), ytog( the_field->defense_area_stretch() / 2.0), dtog(the_field->defense_area_radius()), 3 * PI / 2, PI);
	ctx->line_to(xtog( the_field->length() / 2.0 - the_field->defense_area_radius()), ytog(-the_field->defense_area_stretch() / 2.0));
	ctx->arc_negative(xtog( the_field->length() / 2.0), ytog(-the_field->defense_area_stretch() / 2.0), dtog(the_field->defense_area_radius()), PI, PI / 2);
	ctx->stroke();

	// Draw the players including text.
	const team::ptr teams[2] = {west_team, east_team};
	for (unsigned int i = 0; i < 2; i++) {
		for (unsigned int j = 0; j < teams[i]->size(); j++) {
			if (teams[i]->yellow())
				ctx->set_source_rgb(1.0, 1.0, 0.0);
			else
				ctx->set_source_rgb(0.0, 1.0, 1.0);
			robot::ptr bot = teams[i]->get_robot(j);
			ctx->begin_new_path();
			ctx->arc_negative(xtog(bot->position().x), ytog(bot->position().y), dtog(0.09), atog(bot->orientation() + PI / 4), atog(bot->orientation() - PI / 4));
			ctx->fill();

			if (dragging == bot) {
				ctx->set_source_rgb(1.0, 0.0, 0.0);
				ctx->begin_new_path();
				ctx->arc_negative(xtog(bot->position().x), ytog(bot->position().y), dtog(0.09), atog(bot->orientation() + PI / 4), atog(bot->orientation() - PI / 4));
				ctx->line_to(xtog(bot->position().x + 0.09 * std::cos(bot->orientation() + PI / 4)), ytog(bot->position().y + 0.09 * std::sin(bot->orientation() + PI / 4)));
				ctx->stroke();
			}

			ctx->set_source_rgb(0.0, 0.0, 0.0);
			const Glib::ustring &ustr = Glib::ustring::compose("%1", j);
			const std::string &str = ustr;
			Cairo::TextExtents extents;
			ctx->get_text_extents(str, extents);
			const double x = xtog(bot->position().x) - extents.x_bearing - extents.width / 2.0;
			const double y = ytog(bot->position().y) - extents.y_bearing - extents.height / 2.0;
			ctx->move_to(x, y);
			ctx->show_text(str);

#if DRAW_VELOCITY
			player::ptr plr = player::ptr::cast_dynamic(bot);
			if (plr) {
				ctx->begin_new_path();
				double vx = bot->position().x;
				double vy = bot->position().y;
				ctx->move_to(xtog(vx), ytog(vy));
				const point &reqvel = plr->ui_requested_velocity().rotate(plr->orientation());
				vx += reqvel.x * DRAW_VELOCITY_SCALE;
				vy += reqvel.y * DRAW_VELOCITY_SCALE;
				ctx->line_to(xtog(vx), ytog(vy));
				ctx->stroke();
			}
#endif
		}
	}

	// Draw the velocity vector for the dragged object.
	if (veldragging) {
		ctx->set_source_rgb(0.0, 0.0, 0.0);
		ctx->begin_new_path();
		const point &pos = veldragging->position();
		ctx->move_to(xtog(pos.x), ytog(pos.y));
		ctx->line_to(xtog(pos.x + dragged_velocity.x), ytog(pos.y + dragged_velocity.y));
		ctx->stroke();
	}

	// Draw the ball.
	ctx->set_source_rgb(1.0, 0.5, 0.0);
	ctx->begin_new_path();
	ctx->arc(xtog(the_ball->position().x), ytog(the_ball->position().y), dtog(0.03), 0.0, 2.0 * PI);
	ctx->fill();
	if (dragging == the_ball) {
		ctx->set_source_rgb(1.0, 0.0, 0.0);
		ctx->begin_new_path();
		ctx->arc(xtog(the_ball->position().x), ytog(the_ball->position().y), dtog(0.03), 0.0, 2.0 * PI);
		ctx->stroke();
	}

	// Done.
	return true;
}

void visualizer::update() {
	get_window()->invalidate(false);
}

void visualizer::on_size_allocate(Gtk::Allocation &alloc) {
	int width = alloc.get_width();
	int height = alloc.get_height();
	double xscale = width / (the_field->total_length() * 1.01);
	double yscale = height / (the_field->total_width() * 1.01);
	scale = std::max(std::min(xscale, yscale), 1e-9);
	xtranslate = width / 2.0;
	ytranslate = height / 2.0;
	Gtk::DrawingArea::on_size_allocate(alloc);
}

bool visualizer::on_button_press_event(GdkEventButton *evt) {
	if (!draggable) {
		return true;
	}

	if (evt->type == GDK_BUTTON_PRESS && evt->button == 1) {
		// Calculate the location on the field.
		point click(xtow(evt->x), ytow(evt->y));

		// Clear any currently-dragged object.
		dragging.reset();
		veldragging.reset();

		// Get the new object.
		dragging = object_at(click);

		// Redraw the visualizer.
		update();
	} else if (evt->type == GDK_BUTTON_PRESS && evt->button == 3) {
		// Calculate the location on the field.
		point click(xtow(evt->x), ytow(evt->y));

		// Clear any currently-dragged object.
		dragging.reset();
		veldragging.reset();

		// Get the new object.
		veldragging = object_at(click);

		// Redraw the visualizer.
		update();
	}

	return true;
}

bool visualizer::on_button_release_event(GdkEventButton *evt) {
	if (evt->button == 1) {
		// Drop the object and redraw the visualizer.
		dragging.reset();
		update();
	} else if (evt->button == 3) {
		// Drop the object and redraw the visualizer.
		veldragging.reset();
		update();
	}

	return true;
}

bool visualizer::on_motion_notify_event(GdkEventMotion *evt) {
	if (dragging) {
		// Move the object being dragged.
		dragging->ext_drag(point(xtow(evt->x), ytow(evt->y)), point());
		update();
	} else if (veldragging) {
		// Update the dragging velocity.
		dragged_velocity = point(xtow(evt->x), ytow(evt->y)) - veldragging->position();
		veldragging->ext_drag(veldragging->position(), dragged_velocity);
		update();
	}

	return true;
}

bool visualizer::on_leave_notify_event(GdkEventCrossing *) {
	// Drop the object and redraw the visualizer.
	dragging.reset();
	veldragging.reset();
	update();

	return true;
}

draggable::ptr visualizer::object_at(const point &pos) const {
	// Check if it's a player.
	const team::ptr teams[2] = {west_team, east_team};
	for (unsigned int i = 0; i < 2; i++) {
		for (unsigned int j = 0; j < teams[i]->size(); j++) {
			robot::ptr bot = teams[i]->get_robot(j);
			if ((bot->position() - pos).len() < 0.09)
				return bot;
		}
	}

	// Check if it's the ball.
	if ((the_ball->position() - pos).len() < 0.03)
		return the_ball;

	return draggable::ptr();
}

