#include "uicomponents/visualizer.h"
#include <glibmm/refptr.h>
#include <gtkmm/window.h>
#include <cmath>

Visualizer::Visualizer(Visualizable::World &data)
    : show_field(true),
      show_ball(true),
      show_ball_v(true),
      show_robots(true),
      show_robots_v(false),
      show_robots_dest(true),
      show_robots_path(true),
      show_robots_graphs(true),
      show_overlay(true),
      data(data)
{
    set_size_request(600, 600);
    add_events(Gdk::POINTER_MOTION_MASK);
    add_events(Gdk::BUTTON_PRESS_MASK);
    add_events(Gdk::BUTTON_RELEASE_MASK);
    add_events(Gdk::ENTER_NOTIFY_MASK);
    add_events(Gdk::LEAVE_NOTIFY_MASK);
    update_connection =
        data.signal_tick().connect(sigc::mem_fun(this, &Visualizer::update));
    update_connection.block();
    data.field().signal_changed.connect(
        sigc::mem_fun(this, &Visualizer::compute_scales));
}

void Visualizer::update()
{
    const Glib::RefPtr<Gdk::Window> win(get_window());
    if (win)
    {
        win->invalidate(false);
    }
}

sigc::signal<void, Point> &Visualizer::signal_mouse_moved() const
{
    return signal_mouse_moved_;
}

void Visualizer::on_show()
{
    Gtk::DrawingArea::on_show();
    update_connection.unblock();
}

void Visualizer::on_hide()
{
    Gtk::DrawingArea::on_hide();
    update_connection.block();
}

void Visualizer::on_size_allocate(Gtk::Allocation &alloc)
{
    Gtk::DrawingArea::on_size_allocate(alloc);
    compute_scales();
}

bool Visualizer::on_draw(const Cairo::RefPtr<Cairo::Context> &ctx)
{
    Gtk::DrawingArea::on_draw(ctx);

    // Fill the background with field-green.
    ctx->set_source_rgb(0.0, 0.33, 0.0);
    ctx->paint();

    // If the field data is invalid, go no further.
    if (!data.field().valid())
    {
        return true;
    }

    // Establish the proper transformation from world coordinates to graphical
    // coordinates.
    ctx->translate(xtranslate, ytranslate);
    ctx->scale(scale, -scale);
    ctx->set_line_width(0.01);

    if (show_field)
    {
        // Draw the outline of the referee area.
        ctx->set_source_rgb(0.0, 0.0, 0.0);
        ctx->move_to(
            -data.field().total_length() / 2.0,
            -data.field().total_width() / 2.0);
        ctx->line_to(
            data.field().total_length() / 2.0,
            -data.field().total_width() / 2.0);
        ctx->line_to(
            data.field().total_length() / 2.0,
            data.field().total_width() / 2.0);
        ctx->line_to(
            -data.field().total_length() / 2.0,
            data.field().total_width() / 2.0);
        ctx->line_to(
            -data.field().total_length() / 2.0,
            -data.field().total_width() / 2.0);
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
        ctx->move_to(
            -data.field().length() / 2.0,
            data.field().defense_area_stretch() / 2.0);
        ctx->line_to(
            -data.field().length() / 2.0 + data.field().defense_area_width(),
            data.field().defense_area_stretch() / 2.0);
        ctx->line_to(
            -data.field().length() / 2.0 + data.field().defense_area_width(),
            -data.field().defense_area_stretch() / 2.0);
        ctx->line_to(
            -data.field().length() / 2.0,
            -data.field().defense_area_stretch() / 2.0);
        ctx->stroke();

        // Draw the east defense area.
        ctx->move_to(
            data.field().length() / 2.0,
            data.field().defense_area_stretch() / 2.0);
        ctx->line_to(
            data.field().length() / 2.0 - data.field().defense_area_width(),
            data.field().defense_area_stretch() / 2.0);
        ctx->line_to(
            data.field().length() / 2.0 - data.field().defense_area_width(),
            -data.field().defense_area_stretch() / 2.0);
        ctx->line_to(
            data.field().length() / 2.0,
            -data.field().defense_area_stretch() / 2.0);
        ctx->stroke();

        // Draw the east goal.
        ctx->move_to(
            -data.field().length() / 2.0, data.field().goal_width() / 2.0);
        ctx->line_to(
            -data.field().length() / 2.0 - data.field().goal_width() / 3.0,
            data.field().goal_width() / 2.0);
        ctx->line_to(
            -data.field().length() / 2.0 - data.field().goal_width() / 3.0,
            -data.field().goal_width() / 2.0);
        ctx->line_to(
            -data.field().length() / 2.0, -data.field().goal_width() / 2.0);
        ctx->stroke();

        // Draw the west goal.
        ctx->move_to(
            data.field().length() / 2.0, data.field().goal_width() / 2.0);
        ctx->line_to(
            data.field().length() / 2.0 + data.field().goal_width() / 3.0,
            data.field().goal_width() / 2.0);
        ctx->line_to(
            data.field().length() / 2.0 + data.field().goal_width() / 3.0,
            -data.field().goal_width() / 2.0);
        ctx->line_to(
            data.field().length() / 2.0, -data.field().goal_width() / 2.0);
        ctx->stroke();
    }

    if (show_robots)
    {
        // Set font size for displaying robot pattern indices.
        ctx->set_font_size(0.09);

        // Draw the players including text, and their velocities, destinations,
        // and paths.
        for (unsigned int i = 0; i < data.visualizable_num_robots(); ++i)
        {
            Visualizable::Robot::Ptr bot = data.visualizable_robot(i);
            if (bot)
            {
                const Visualizable::Colour &clr = bot->visualizer_colour();
                const std::string &str          = bot->visualizer_label();
                Cairo::TextExtents extents;
                ctx->get_text_extents(str, extents);

                ctx->set_source_rgb(clr.red, clr.green, clr.blue);
                ctx->begin_new_path();
                ctx->arc(
                    bot->position().x, bot->position().y, 0.09,
                    bot->orientation().to_radians() + M_PI_4,
                    bot->orientation().to_radians() - M_PI_4);
                ctx->fill();

                if (show_robots_v)
                {
                    ctx->begin_new_path();
                    ctx->move_to(bot->position().x, bot->position().y);
                    const Point &tgt(bot->position() + bot->velocity());
                    ctx->line_to(tgt.x, tgt.y);
                    ctx->stroke();
                }

                if (show_robots_path && bot->has_display_path())
                {
                    ctx->set_source_rgb(1.0, 0.0, 1.0);
                    ctx->begin_new_path();
                    ctx->move_to(bot->position().x, bot->position().y);
                    const std::vector<Point> &path = bot->display_path();
                    for (Point j : path)
                    {
                        ctx->line_to(j.x, j.y);
                    }
                    ctx->stroke();

                    ctx->set_source_rgb(0, 0, 0);
                    for (Point j : path)
                    {
                        ctx->arc(j.x, j.y, 0.01, 0, 2 * M_PI);
                        ctx->fill();
                    }
                }

                if (show_robots_graphs)
                {
                    unsigned int count = bot->num_bar_graphs();
                    if (count)
                    {
                        const double BAR_HEIGHT = 0.1;
                        const double BAR_WIDTH  = 0.4;
                        double tlx = bot->position().x - BAR_WIDTH / 2.0;
                        double tly =
                            bot->position().y - BAR_HEIGHT * (count + 1) - 0.09;
                        ctx->set_source_rgb(0.0, 0.0, 0.0);
                        ctx->begin_new_path();
                        ctx->move_to(tlx, tly);
                        ctx->line_to(tlx + BAR_WIDTH, tly);
                        ctx->line_to(tlx + BAR_WIDTH, tly + BAR_HEIGHT * count);
                        ctx->line_to(tlx, tly + BAR_HEIGHT * count);
                        ctx->fill();

                        for (unsigned int i = 0; i < count; ++i)
                        {
                            double value = bot->bar_graph_value(i);
                            const Visualizable::Colour &graphclr =
                                bot->bar_graph_colour(i);
                            double bar_tly = tly + BAR_HEIGHT * i;
                            ctx->set_source_rgb(
                                graphclr.red, graphclr.green, graphclr.blue);
                            ctx->begin_new_path();
                            ctx->move_to(
                                tlx + BAR_WIDTH * 0.15,
                                bar_tly + BAR_HEIGHT * 0.15);
                            ctx->line_to(
                                tlx + BAR_WIDTH * (0.15 + 0.7 * value),
                                bar_tly + BAR_HEIGHT * 0.15);
                            ctx->line_to(
                                tlx + BAR_WIDTH * (0.15 + 0.7 * value),
                                bar_tly + BAR_HEIGHT * 0.85);
                            ctx->line_to(
                                tlx + BAR_WIDTH * 0.15,
                                bar_tly + BAR_HEIGHT * 0.85);
                            ctx->fill();
                        }
                    }
                }

                ctx->set_source_rgb(0.0, 0.0, 0.0);
                const double x =
                    bot->position().x - extents.x_bearing - extents.width / 2.0;
                const double y = bot->position().y + extents.y_bearing +
                                 extents.height / 2.0;
                ctx->move_to(x, y);
                ctx->save();
                ctx->scale(1, -1);
                ctx->show_text(str);
                ctx->restore();

                if (bot->highlight())
                {
                    const Visualizable::Colour &hlclr = bot->highlight_colour();
                    ctx->set_source_rgb(hlclr.red, hlclr.green, hlclr.blue);
                    ctx->begin_new_path();
                    ctx->arc(
                        bot->position().x, bot->position().y, 0.09,
                        bot->orientation().to_radians() + M_PI_4,
                        bot->orientation().to_radians() - M_PI_4);
                    ctx->stroke();
                }
            }
        }
    }

    if (show_ball)
    {
        // Draw the ball.
        const Visualizable::Ball &ball(data.ball());
        ctx->set_source_rgb(1.0, 0.5, 0.0);
        ctx->begin_new_path();
        ctx->arc(ball.position().x, ball.position().y, 0.03, 0, 2 * M_PI);
        ctx->fill();

        if (show_ball_v)
        {
            ctx->begin_new_path();
            ctx->move_to(ball.position().x, ball.position().y);
            const Point &tgt(ball.position() + ball.velocity());
            ctx->line_to(tgt.x, tgt.y);
            ctx->stroke();
        }

        if (ball.highlight())
        {
            const Visualizable::Colour &clr = ball.highlight_colour();
            ctx->set_source_rgb(clr.red, clr.green, clr.blue);
            ctx->begin_new_path();
            ctx->arc(ball.position().x, ball.position().y, 0.03, 0, 2 * M_PI);
            ctx->stroke();
        }
    }

    if (show_overlay)
    {
        // Draw the overlay.
        data.draw_overlay(ctx);
    }

    // Done.
    return true;
}

bool Visualizer::on_button_press_event(GdkEventButton *evt)
{
    data.mouse_pressed(Point(xtow(evt->x), ytow(evt->y)), evt->button);
    return true;
}

bool Visualizer::on_button_release_event(GdkEventButton *evt)
{
    data.mouse_released(Point(xtow(evt->x), ytow(evt->y)), evt->button);
    return true;
}

bool Visualizer::on_motion_notify_event(GdkEventMotion *evt)
{
    Point p(xtow(evt->x), ytow(evt->y));
    data.mouse_moved(p);
    signal_mouse_moved().emit(p);
    return true;
}

bool Visualizer::on_leave_notify_event(GdkEventCrossing *)
{
    data.mouse_exited();
    return true;
}

void Visualizer::compute_scales()
{
    if (data.field().valid())
    {
        int width     = get_width();
        int height    = get_height();
        double xscale = width / (data.field().total_length() * 1.01);
        double yscale = height / (data.field().total_width() * 1.01);
        scale         = std::max(std::min(xscale, yscale), 1e-9);
        xtranslate    = width / 2.0;
        ytranslate    = height / 2.0;
    }
}
