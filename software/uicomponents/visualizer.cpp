// Set nonzero to draw velocity vectors.
#define DRAW_VELOCITY 0
// Set to the multiplier for visualizing velocity vector lengths.
#define DRAW_VELOCITY_SCALE 1.0

#define DEBUG 0
#include "uicomponents/visualizer.h"
#include "util/dprint.h"
#include <cmath>

visualizer::visualizer(const visualizable &data) : data(data) {
	set_size_request(600, 600);
	add_events(Gdk::POINTER_MOTION_MASK);
	add_events(Gdk::BUTTON_PRESS_MASK);
	add_events(Gdk::BUTTON_RELEASE_MASK);
	add_events(Gdk::ENTER_NOTIFY_MASK);
	add_events(Gdk::LEAVE_NOTIFY_MASK);
	update_connection = data.signal_visdata_changed.connect(sigc::mem_fun(this, &visualizer::update));
	update_connection.block();
}

void visualizer::on_show() {
	Gtk::DrawingArea::on_show();
	update_connection.unblock();
}

void visualizer::on_hide() {
	Gtk::DrawingArea::on_hide();
	update_connection.block();
}

bool visualizer::on_expose_event(GdkEventExpose *evt) {
	DPRINT("Enter on_expose_event.");

	Gtk::DrawingArea::on_expose_event(evt);

	int width, height;
	get_window()->get_size(width, height);

	Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();
	ctx->set_font_size(dtog(0.09));

	// Fill the background with field-green.
	ctx->set_source_rgb(0.0, 0.33, 0.0);
	ctx->move_to(0.0, 0.0);
	ctx->line_to(width, 0.0);
	ctx->line_to(width, height);
	ctx->line_to(0.0, height);
	ctx->fill();

	// If the field data is invalid, go no further.
	if (!data.field().valid()) {
		DPRINT("Exit on_expose_event (1).");
		return true;
	}

	// Draw the outline of the referee area.
	ctx->set_source_rgb(0.0, 0.0, 0.0);
	ctx->move_to(xtog(-data.field().total_length() / 2.0), ytog(-data.field().total_width() / 2.0));
	ctx->line_to(xtog( data.field().total_length() / 2.0), ytog(-data.field().total_width() / 2.0));
	ctx->line_to(xtog( data.field().total_length() / 2.0), ytog( data.field().total_width() / 2.0));
	ctx->line_to(xtog(-data.field().total_length() / 2.0), ytog( data.field().total_width() / 2.0));
	ctx->line_to(xtog(-data.field().total_length() / 2.0), ytog(-data.field().total_width() / 2.0));
	ctx->stroke();

	// Draw the rectangular outline.
	ctx->set_source_rgb(1.0, 1.0, 1.0);
	ctx->move_to(xtog(-data.field().length() / 2.0), ytog(-data.field().width() / 2.0));
	ctx->line_to(xtog( data.field().length() / 2.0), ytog(-data.field().width() / 2.0));
	ctx->line_to(xtog( data.field().length() / 2.0), ytog( data.field().width() / 2.0));
	ctx->line_to(xtog(-data.field().length() / 2.0), ytog( data.field().width() / 2.0));
	ctx->line_to(xtog(-data.field().length() / 2.0), ytog(-data.field().width() / 2.0));
	ctx->stroke();

	// Draw the centre line.
	ctx->move_to(xtog(0.0), ytog(-data.field().width() / 2.0));
	ctx->line_to(xtog(0.0), ytog( data.field().width() / 2.0));
	ctx->stroke();

	// Draw the centre circle.
	ctx->arc(xtog(0.0), ytog(0.0), dtog(data.field().centre_circle_radius()), 0.0, 2 * M_PI);
	ctx->stroke();

	// Draw the west defense area.
	ctx->arc(xtog(-data.field().length() / 2.0), ytog( data.field().defense_area_stretch() / 2.0), dtog(data.field().defense_area_radius()), 3 * M_PI_2, 2 * M_PI);
	ctx->line_to(xtog(-data.field().length() / 2.0 + data.field().defense_area_radius()), ytog( data.field().defense_area_stretch() / 2.0));
	ctx->arc(xtog(-data.field().length() / 2.0), ytog(-data.field().defense_area_stretch() / 2.0), dtog(data.field().defense_area_radius()), 0, M_PI_2);
	ctx->stroke();

	// Draw the east defense area.
	ctx->arc_negative(xtog( data.field().length() / 2.0), ytog( data.field().defense_area_stretch() / 2.0), dtog(data.field().defense_area_radius()), 3 * M_PI_2, M_PI);
	ctx->line_to(xtog( data.field().length() / 2.0 - data.field().defense_area_radius()), ytog(-data.field().defense_area_stretch() / 2.0));
	ctx->arc_negative(xtog( data.field().length() / 2.0), ytog(-data.field().defense_area_stretch() / 2.0), dtog(data.field().defense_area_radius()), M_PI, M_PI_2);
	ctx->stroke();

	// Draw the players including text.
	for (unsigned int i = 0; i < data.size(); ++i) {
		const visualizable::robot::ptr bot(data[i]);
		const visualizable::colour &clr(bot->visualizer_colour());
		ctx->set_source_rgb(clr.red, clr.green, clr.blue);
		ctx->begin_new_path();
		ctx->arc_negative(xtog(bot->position().x), ytog(bot->position().y), dtog(0.09), atog(bot->orientation() + M_PI_4), atog(bot->orientation() - M_PI_4));
		ctx->fill();

#warning IMPLEMENT DRAGGING
#if 0
		if (dragging == bot) {
			ctx->set_source_rgb(1.0, 0.0, 0.0);
			ctx->begin_new_path();
			ctx->arc_negative(xtog(bot->position().x), ytog(bot->position().y), dtog(0.09), atog(bot->orientation() + M_PI_4), atog(bot->orientation() - M_PI_4));
			ctx->line_to(xtog(bot->position().x + 0.09 * std::cos(bot->orientation() + M_PI_4)), ytog(bot->position().y + 0.09 * std::sin(bot->orientation() + M_PI_4)));
			ctx->stroke();
		}
#endif

		ctx->set_source_rgb(0.0, 0.0, 0.0);
		const Glib::ustring &ustr(bot->visualizer_label());
		const std::string &str(ustr);
		Cairo::TextExtents extents;
		ctx->get_text_extents(str, extents);
		const double x = xtog(bot->position().x) - extents.x_bearing - extents.width / 2.0;
		const double y = ytog(bot->position().y) - extents.y_bearing - extents.height / 2.0;
		ctx->move_to(x, y);
		ctx->show_text(str);

#warning IMPLEMENT VELOCITY DRAWING
#if DRAW_VELOCITY && 0
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

#warning IMPLEMENT VELOCITY DRAGGING
#if 0
	// Draw the velocity vector for the dragged object.
	if (veldragging) {
		ctx->set_source_rgb(0.0, 0.0, 0.0);
		ctx->begin_new_path();
		const point &pos = veldragging->position();
		ctx->move_to(xtog(pos.x), ytog(pos.y));
		ctx->line_to(xtog(pos.x + dragged_velocity.x), ytog(pos.y + dragged_velocity.y));
		ctx->stroke();
	}
#endif

	// Draw the ball.
	const visualizable::ball::ptr the_ball(data.ball());
	ctx->set_source_rgb(1.0, 0.5, 0.0);
	ctx->begin_new_path();
	DPRINT(Glib::ustring::compose("The ball is at (%1, %2) which is (%1, %2) in graphical coordinates with radius (%3).", the_ball->position().x, the_ball->position().y, xtog(the_ball->position().x), ytog(the_ball->position().y), dtog(0.03)));
	ctx->arc(xtog(the_ball->position().x), ytog(the_ball->position().y), dtog(0.03), 0.0, 2.0 * M_PI);
	ctx->fill();
#warning IMPLEMENT DRAGGING
#if 0
	if (dragging == the_ball) {
		ctx->set_source_rgb(1.0, 0.0, 0.0);
		ctx->begin_new_path();
		ctx->arc(xtog(the_ball->position().x), ytog(the_ball->position().y), dtog(0.03), 0.0, 2.0 * M_PI);
		ctx->stroke();
	}
#endif

	// Done.
	DPRINT("Exit on_expose_event (2).");
	return true;
}

void visualizer::update() {
	const Glib::RefPtr<Gdk::Window> win(get_window());
	if (win) {
		win->invalidate(false);
	}
}

void visualizer::on_size_allocate(Gtk::Allocation &alloc) {
	Gtk::DrawingArea::on_size_allocate(alloc);

	int width = alloc.get_width();
	int height = alloc.get_height();
	double xscale = width / (data.field().total_length() * 1.01);
	double yscale = height / (data.field().total_width() * 1.01);
	scale = std::max(std::min(xscale, yscale), 1e-9);
	xtranslate = width / 2.0;
	ytranslate = height / 2.0;
	Gtk::DrawingArea::on_size_allocate(alloc);
}

bool visualizer::on_button_press_event(GdkEventButton *evt) {
	Gtk::DrawingArea::on_button_press_event(evt);
#warning IMPLEMENT DRAGGING
#if 0
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

#endif
	return true;
}

bool visualizer::on_button_release_event(GdkEventButton *evt) {
	Gtk::DrawingArea::on_button_release_event(evt);
#warning IMPLEMENT DRAGGING
#if 0
	if (evt->button == 1) {
		// Drop the object and redraw the visualizer.
		dragging.reset();
		update();
	} else if (evt->button == 3) {
		// Drop the object and redraw the visualizer.
		veldragging.reset();
		update();
	}

#endif
	return true;
}

bool visualizer::on_motion_notify_event(GdkEventMotion *evt) {
	Gtk::DrawingArea::on_motion_notify_event(evt);
#warning IMPLEMENT DRAGGING
#if 0
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

#endif
	return true;
}

bool visualizer::on_leave_notify_event(GdkEventCrossing *evt) {
	Gtk::DrawingArea::on_leave_notify_event(evt);
#warning IMPLEMENT DRAGGING
#if 0
	// Drop the object and redraw the visualizer.
	dragging.reset();
	veldragging.reset();
	update();

#endif
	return true;
}

#warning IMPLEMENT DRAGGING
#if 0
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
#endif

