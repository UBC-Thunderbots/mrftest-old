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

void Visualizer::on_show() {
	Gtk::DrawingArea::on_show();
	update_connection.unblock();
}

void Visualizer::on_hide() {
	Gtk::DrawingArea::on_hide();
	update_connection.block();
	signal_overlay_changed.emit();
}

bool Visualizer::on_expose_event(GdkEventExpose *evt) {
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
		return true;
	}

	// Draw the outline of the referee area.
	ctx->set_source_rgb(0.0, 0.0, 0.0);
	ctx->move_to(xtog(-data.field().total_length() / 2.0), ytog(-data.field().total_width() / 2.0));
	ctx->line_to(xtog(data.field().total_length() / 2.0), ytog(-data.field().total_width() / 2.0));
	ctx->line_to(xtog(data.field().total_length() / 2.0), ytog(data.field().total_width() / 2.0));
	ctx->line_to(xtog(-data.field().total_length() / 2.0), ytog(data.field().total_width() / 2.0));
	ctx->line_to(xtog(-data.field().total_length() / 2.0), ytog(-data.field().total_width() / 2.0));
	ctx->stroke();

	// Draw the rectangular outline.
	ctx->set_source_rgb(1.0, 1.0, 1.0);
	ctx->move_to(xtog(-data.field().length() / 2.0), ytog(-data.field().width() / 2.0));
	ctx->line_to(xtog(data.field().length() / 2.0), ytog(-data.field().width() / 2.0));
	ctx->line_to(xtog(data.field().length() / 2.0), ytog(data.field().width() / 2.0));
	ctx->line_to(xtog(-data.field().length() / 2.0), ytog(data.field().width() / 2.0));
	ctx->line_to(xtog(-data.field().length() / 2.0), ytog(-data.field().width() / 2.0));
	ctx->stroke();

	// Draw the centre line.
	ctx->move_to(xtog(0.0), ytog(-data.field().width() / 2.0));
	ctx->line_to(xtog(0.0), ytog(data.field().width() / 2.0));
	ctx->stroke();

	// Draw the centre circle.
	ctx->arc(xtog(0.0), ytog(0.0), dtog(data.field().centre_circle_radius()), 0.0, 2 * M_PI);
	ctx->stroke();

	// Draw the west defense area.
	ctx->arc(xtog(-data.field().length() / 2.0), ytog(data.field().defense_area_stretch() / 2.0), dtog(data.field().defense_area_radius()), 3 * M_PI_2, 2 * M_PI);
	ctx->line_to(xtog(-data.field().length() / 2.0 + data.field().defense_area_radius()), ytog(data.field().defense_area_stretch() / 2.0));
	ctx->arc(xtog(-data.field().length() / 2.0), ytog(-data.field().defense_area_stretch() / 2.0), dtog(data.field().defense_area_radius()), 0, M_PI_2);
	ctx->stroke();

	// Draw the east defense area.
	ctx->arc_negative(xtog(data.field().length() / 2.0), ytog(data.field().defense_area_stretch() / 2.0), dtog(data.field().defense_area_radius()), 3 * M_PI_2, M_PI);
	ctx->line_to(xtog(data.field().length() / 2.0 - data.field().defense_area_radius()), ytog(-data.field().defense_area_stretch() / 2.0));
	ctx->arc_negative(xtog(data.field().length() / 2.0), ytog(-data.field().defense_area_stretch() / 2.0), dtog(data.field().defense_area_radius()), M_PI, M_PI_2);
	ctx->stroke();

	// Draw the players including text.
	for (unsigned int i = 0; i < data.visualizable_num_robots(); ++i) {
		Visualizable::Robot::Ptr bot = data.visualizable_robot(i);
		const Visualizable::RobotColour &clr(bot->visualizer_colour());
		ctx->set_source_rgb(clr.red, clr.green, clr.blue);
		ctx->begin_new_path();
		ctx->arc_negative(xtog(bot->position().x), ytog(bot->position().y), dtog(0.09), atog(bot->orientation() + M_PI_4), atog(bot->orientation() - M_PI_4));
		ctx->fill();

		ctx->set_source_rgb(0.0, 0.0, 0.0);
		const Glib::ustring &ustr(bot->visualizer_label());
		const std::string &str(ustr);
		Cairo::TextExtents extents;
		ctx->get_text_extents(str, extents);
		const double x = xtog(bot->position().x) - extents.x_bearing - extents.width / 2.0;
		const double y = ytog(bot->position().y) - extents.y_bearing - extents.height / 2.0;
		ctx->move_to(x, y);
		ctx->show_text(str);
	}

	// Draw the ball.
	const Visualizable::Ball &ball(data.ball());
	ctx->set_source_rgb(1.0, 0.5, 0.0);
	ctx->begin_new_path();
	ctx->arc(xtog(ball.position().x), ytog(ball.position().y), dtog(0.03), 0.0, 2.0 * M_PI);
	ctx->fill();
	ctx->begin_new_path();
	ctx->move_to(xtog(ball.position().x), ytog(ball.position().y));
	{
		const Point &tgt(ball.position() + ball.velocity());
		ctx->line_to(xtog(tgt.x), ytog(tgt.y));
	}
	ctx->stroke();

	// Done.
	return true;
}

void Visualizer::update() {
	const Glib::RefPtr<Gdk::Window> win(get_window());
	if (win) {
		win->invalidate(false);
	}
}

void Visualizer::on_size_allocate(Gtk::Allocation &alloc) {
	Gtk::DrawingArea::on_size_allocate(alloc);
	compute_scales();
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
		signal_overlay_changed.emit();
	}
}

