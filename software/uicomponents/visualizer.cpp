#include "uicomponents/visualizer.h"
#include <cmath>

Visualizer::Visualizer(Visualizable::World &data) : data(data) {
	set_size_request(600, 600);
	add_events(Gdk::POINTER_MOTION_MASK);
	add_events(Gdk::BUTTON_PRESS_MASK);
	add_events(Gdk::BUTTON_RELEASE_MASK);
	add_events(Gdk::ENTER_NOTIFY_MASK);
	add_events(Gdk::LEAVE_NOTIFY_MASK);
	update_connection = data.signal_tick().connect(sigc::mem_fun(this, &Visualizer::update));
	update_connection.block();
	data.field().signal_changed.connect(sigc::mem_fun(this, &Visualizer::compute_scales));
}

void Visualizer::update() {
	const Glib::RefPtr<Gdk::Window> win(get_window());
	if (win) {
		win->invalidate(false);
	}
}

void Visualizer::on_show() {
	Gtk::DrawingArea::on_show();
	update_connection.unblock();
}

void Visualizer::on_hide() {
	Gtk::DrawingArea::on_hide();
	update_connection.block();
}

void Visualizer::on_size_allocate(Gtk::Allocation &alloc) {
	Gtk::DrawingArea::on_size_allocate(alloc);
	compute_scales();
}

bool Visualizer::on_expose_event(GdkEventExpose *evt) {
	Gtk::DrawingArea::on_expose_event(evt);

	int width, height;
	get_window()->get_size(width, height);

	Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();

	// Fill the background with field-green.
	ctx->set_source_rgb(0.0, 0.33, 0.0);
	ctx->move_to(0.0, 0.0);
	ctx->line_to(width, 0.0);
	ctx->line_to(width, height);
	ctx->line_to(0.0, height);
	ctx->fill();

	// If the field data is invalid, go no further.
	if (!data.field().valid()) {
		return true;
	}

	// Establish the proper transformation from world coordinates to graphical coordinates.
	ctx->translate(xtranslate, ytranslate);
	ctx->scale(scale, -scale);
	ctx->set_line_width(0.01);

	// Draw the outline of the referee area.
	ctx->set_source_rgb(0.0, 0.0, 0.0);
	ctx->move_to(-data.field().total_length() / 2.0, -data.field().total_width() / 2.0);
	ctx->line_to(data.field().total_length() / 2.0, -data.field().total_width() / 2.0);
	ctx->line_to(data.field().total_length() / 2.0, data.field().total_width() / 2.0);
	ctx->line_to(-data.field().total_length() / 2.0, data.field().total_width() / 2.0);
	ctx->line_to(-data.field().total_length() / 2.0, -data.field().total_width() / 2.0);
	ctx->stroke();

	// Draw the rectangular outline.
	ctx->set_source_rgb(1.0, 1.0, 1.0);
	ctx->move_to(-data.field().length() / 2.0, -data.field().width() / 2.0);
	ctx->line_to(data.field().length() / 2.0, -data.field().width() / 2.0);
	ctx->line_to(data.field().length() / 2.0, data.field().width() / 2.0);
	ctx->line_to(-data.field().length() / 2.0, data.field().width() / 2.0);
	ctx->line_to(-data.field().length() / 2.0, -data.field().width() / 2.0);
	ctx->stroke();

	// Draw the centre line.
	ctx->move_to(0.0, -data.field().width() / 2.0);
	ctx->line_to(0.0, data.field().width() / 2.0);
	ctx->stroke();

	// Draw the centre circle.
	ctx->arc(0.0, 0.0, data.field().centre_circle_radius(), 0.0, 2 * M_PI);
	ctx->stroke();

	// Draw the west defense area.
	ctx->arc(-data.field().length() / 2.0, -data.field().defense_area_stretch() / 2.0, data.field().defense_area_radius(), -M_PI_2, 0);
	ctx->line_to(-data.field().length() / 2.0 + data.field().defense_area_radius(), data.field().defense_area_stretch() / 2.0);
	ctx->arc(-data.field().length() / 2.0, data.field().defense_area_stretch() / 2.0, data.field().defense_area_radius(), 0, M_PI_2);
	ctx->stroke();

	// Draw the east defense area.
	ctx->arc(data.field().length() / 2.0, data.field().defense_area_stretch() / 2.0, data.field().defense_area_radius(), M_PI_2, M_PI);
	ctx->line_to(data.field().length() / 2.0 - data.field().defense_area_radius(), -data.field().defense_area_stretch() / 2.0);
	ctx->arc(data.field().length() / 2.0, -data.field().defense_area_stretch() / 2.0, data.field().defense_area_radius(), M_PI, 3 * M_PI_2);
	ctx->stroke();

	// Set font size for displaying robot pattern indices.
	ctx->set_font_size(0.09);

	// Draw the players including text, and their destinations.
	for (unsigned int i = 0; i < data.visualizable_num_robots(); ++i) {
		Visualizable::Robot::Ptr bot = data.visualizable_robot(i);
		const Visualizable::Colour &clr = bot->visualizer_colour();
		const std::string &str = bot->visualizer_label();
		Cairo::TextExtents extents;
		ctx->get_text_extents(str, extents);

		if (bot->has_destination()) {
			ctx->set_source_rgb(clr.red, clr.green, clr.blue);
			ctx->begin_new_path();
			ctx->arc(bot->destination().first.x, bot->destination().first.y, 0.09, bot->destination().second + M_PI_4, bot->destination().second - M_PI_4);
			ctx->stroke();

			ctx->set_source_rgb(0.0, 0.0, 0.0);
			const double x = bot->destination().first.x - extents.x_bearing - extents.width / 2.0;
			const double y = bot->destination().first.y + extents.y_bearing + extents.height / 2.0;
			ctx->move_to(x, y);
			ctx->save();
			ctx->scale(1, -1);
			ctx->show_text(str);
			ctx->restore();
		}

		if (bot->has_path()) {
			ctx->set_source_rgb(clr.red, clr.green, clr.blue);
			ctx->begin_new_path();
			ctx->move_to(bot->position().x, bot->position().y);
			const std::vector<std::pair<std::pair<Point, double>, timespec> > &path = bot->path();
			for (std::vector<std::pair<std::pair<Point, double>, timespec> >::const_iterator j = path.begin(), jend = path.end(); j != jend; ++j) {
				ctx->line_to(j->first.first.x, j->first.first.y);
			}
			ctx->stroke();

			ctx->set_source_rgb(0, 0, 0);
			for (std::vector<std::pair<std::pair<Point, double>, timespec> >::const_iterator j = path.begin(), jend = path.end(); j != jend; ++j) {
				ctx->arc(j->first.first.x, j->first.first.y, 0.01, 0, 2 * M_PI);
				ctx->fill();
			}
		}

		ctx->set_source_rgb(clr.red, clr.green, clr.blue);
		ctx->begin_new_path();
		ctx->arc(bot->position().x, bot->position().y, 0.09, bot->orientation() + M_PI_4, bot->orientation() - M_PI_4);
		ctx->fill();

		ctx->set_source_rgb(0.0, 0.0, 0.0);
		const double x = bot->position().x - extents.x_bearing - extents.width / 2.0;
		const double y = bot->position().y + extents.y_bearing + extents.height / 2.0;
		ctx->move_to(x, y);
		ctx->save();
		ctx->scale(1, -1);
		ctx->show_text(str);
		ctx->restore();

		if (bot->highlight()) {
			const Visualizable::Colour &hlclr = bot->highlight_colour();
			ctx->set_source_rgb(hlclr.red, hlclr.green, hlclr.blue);
			ctx->begin_new_path();
			ctx->arc(bot->position().x, bot->position().y, 0.09, bot->orientation() + M_PI_4, bot->orientation() - M_PI_4);
			ctx->stroke();
		}
	}

	// Draw the ball.
	const Visualizable::Ball &ball(data.ball());
	ctx->set_source_rgb(1.0, 0.5, 0.0);
	ctx->begin_new_path();
	ctx->arc(ball.position().x, ball.position().y, 0.03, 0, 2 * M_PI);
	ctx->fill();
	ctx->begin_new_path();
	ctx->move_to(ball.position().x, ball.position().y);
	{
		const Point &tgt(ball.position() + ball.velocity());
		ctx->line_to(tgt.x, tgt.y);
	}
	ctx->stroke();
	if (ball.highlight()) {
		const Visualizable::Colour &clr = ball.highlight_colour();
		ctx->set_source_rgb(clr.red, clr.green, clr.blue);
		ctx->begin_new_path();
		ctx->arc(ball.position().x, ball.position().y, 0.03, 0, 2 * M_PI);
		ctx->stroke();
	}

	// Done.
	return true;
}

bool Visualizer::on_button_press_event(GdkEventButton *evt) {
	data.mouse_pressed(Point(xtow(evt->x), ytow(evt->y)), evt->button);
	return true;
}

bool Visualizer::on_button_release_event(GdkEventButton *evt) {
	data.mouse_released(Point(xtow(evt->x), ytow(evt->y)), evt->button);
	return true;
}

bool Visualizer::on_motion_notify_event(GdkEventMotion *evt) {
	data.mouse_moved(Point(xtow(evt->x), ytow(evt->y)));
	return true;
}

bool Visualizer::on_leave_notify_event(GdkEventCrossing *) {
	data.mouse_exited();
	return true;
}

void Visualizer::compute_scales() {
	if (data.field().valid()) {
		int width = get_width();
		int height = get_height();
		double xscale = width / (data.field().total_length() * 1.01);
		double yscale = height / (data.field().total_width() * 1.01);
		scale = std::max(std::min(xscale, yscale), 1e-9);
		xtranslate = width / 2.0;
		ytranslate = height / 2.0;
	}
}

