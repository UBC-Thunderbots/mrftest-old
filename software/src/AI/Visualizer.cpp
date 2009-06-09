#include "AI/Visualizer.h"
#include "datapool/World.h"

#include <sstream>
#include <cairomm/context.h> 
#include <gtkmm/main.h>

Visualizer::Visualizer() {
	win.set_title("Thunderbots Simulator");
	win.set_size_request(660, 470);
	win.add(*this);
	show();
	win.show();
}

bool Visualizer::on_expose_event(GdkEventExpose *event) {
	// This is where we draw on the window
	Glib::RefPtr<Gdk::Window> window = get_window();
	if (!window)
		return true;

	Gtk::Allocation allocation = get_allocation();

	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();

	if (event) {
		// clip to the area indicated by the expose event so that we only
		// redraw the portion of the window that needs to be redrawn
		cr->rectangle(event->area.x, event->area.y,
				event->area.width, event->area.height);
		cr->clip();
	}
	cr->set_line_width(0.5);

	// green:
	cr->save();
	cr->set_source_rgba(0.337, 0.612, 0.117, 0.9);
	cr->paint();
	cr->restore();

	World &w = World::get();
	PField field = w.field();

	// lines:
	cr->set_source_rgb (1, 1, 1);
	
	double scaling = 660.0 / field->width();
	double offsetX = (-field->west() + 25) * scaling;
	double offsetY = (-field->north() + 25) * scaling;

	// north:
	cr->move_to(offsetX+scaling*field->west(), offsetY+scaling*field->north());
	cr->line_to(offsetX+scaling*field->east(), offsetY+scaling*field->north());

	// west:
	cr->move_to(offsetX+scaling*field->west(), offsetY+scaling*field->north());
	cr->line_to(offsetX+scaling*field->west(), offsetY+scaling*field->south());

	// south:
	cr->move_to(offsetX+scaling*field->west(), offsetY+scaling*field->south());
	cr->line_to(offsetX+scaling*field->east(), offsetY+scaling*field->south());

	// east:
	cr->move_to(offsetX+scaling*field->east(), offsetY+scaling*field->north());
	cr->line_to(offsetX+scaling*field->east(), offsetY+scaling*field->south());

	// center:
	cr->move_to(offsetX+scaling*field->centerCircle().x, offsetY+scaling*field->north());
	cr->line_to(offsetX+scaling*field->centerCircle().x, offsetY+scaling*field->south());

	// west goal:
	cr->move_to(offsetX+scaling*field->westGoal()->north.x,      offsetY+scaling*field->westGoal()->north.y);
	cr->line_to(offsetX+scaling*(field->westGoal()->north.x - 10), offsetY+scaling*field->westGoal()->north.y);
	cr->move_to(offsetX+scaling*(field->westGoal()->north.x - 10), offsetY+scaling*field->westGoal()->north.y);
	cr->line_to(offsetX+scaling*(field->westGoal()->south.x - 10), offsetY+scaling*field->westGoal()->south.y);
	cr->move_to(offsetX+scaling*(field->westGoal()->south.x - 10), offsetY+scaling*field->westGoal()->south.y);
	cr->line_to(offsetX+scaling*field->westGoal()->south.x,      offsetY+scaling*field->westGoal()->south.y);

	// east goal:
	cr->move_to(offsetX+scaling*field->eastGoal()->north.x,      offsetY+scaling*field->eastGoal()->north.y);
	cr->line_to(offsetX+scaling*(field->eastGoal()->north.x + 10), offsetY+scaling*field->eastGoal()->north.y);
	cr->move_to(offsetX+scaling*(field->eastGoal()->north.x + 10), offsetY+scaling*field->eastGoal()->north.y);
	cr->line_to(offsetX+scaling*(field->eastGoal()->south.x + 10), offsetY+scaling*field->eastGoal()->south.y);
	cr->move_to(offsetX+scaling*(field->eastGoal()->south.x + 10), offsetY+scaling*field->eastGoal()->south.y);
	cr->line_to(offsetX+scaling*field->eastGoal()->south.x,      offsetY+scaling*field->eastGoal()->south.y);

	// west defense area:
	double rad = scaling*(field->westGoal()->defenseN.x - field->west());
	cr->move_to(offsetX+scaling*field->west(), offsetY+scaling*(field->westGoal()->defenseN.y) - rad);
	cr->arc(offsetX+scaling*field->west(), offsetY+scaling*field->westGoal()->defenseN.y, rad, 1.5 * M_PI, 0.0);
	cr->line_to(offsetX+scaling*field->westGoal()->defenseS.x, offsetY+scaling*field->westGoal()->defenseS.y);
	cr->arc(offsetX+scaling*field->west(), offsetY+scaling*field->westGoal()->defenseS.y, rad, 0.0, M_PI / 2.0); 

	// east defense area:
	rad = scaling*(field->east() - field->eastGoal()->defenseN.x);

	cr->move_to(offsetX+scaling*field->east(), offsetY+scaling*field->eastGoal()->defenseS.y + rad);
	cr->arc(offsetX+scaling*field->east(), offsetY+scaling*field->eastGoal()->defenseS.y, rad, M_PI / 2.0, M_PI);
	cr->line_to(offsetX+scaling*field->eastGoal()->defenseN.x, offsetY+scaling*field->eastGoal()->defenseN.y);
	cr->arc(offsetX+scaling*field->east(), offsetY+scaling*field->eastGoal()->defenseN.y, rad, M_PI, 1.5 * M_PI);

	cr->stroke();

	// center circle:
	cr->arc(offsetX+scaling*field->centerCircle().x, offsetY+scaling*field->centerCircle().y, scaling*field->centerCircleRadius(), 0.0, 2.0 * M_PI);

	cr->stroke();

	// score
	cr->set_font_size(50.0);
	std::ostringstream ss;
	ss << w.friendlyTeam()->score();
	if (w.friendlyTeam()->side())
		cr->move_to(150.0, 100.0);
	else
		cr->move_to(470.0, 100.0);
	cr->text_path(ss.str());
	cr->set_source_rgba(1, 0.2, 0.2, 0.5);
	cr->fill_preserve();
	cr->set_source_rgba(0, 0, 0, 0.5);
	cr->set_line_width(1);
	cr->stroke();

	ss.str("");
	ss << w.enemyTeam()->score();
	if (w.enemyTeam()->side())
		cr->move_to(150.0, 100.0);
	else
		cr->move_to(470.0, 100.0);
	cr->text_path(ss.str());
	cr->set_source_rgba(0.2, 0.2, 1, 0.5);
	cr->fill_preserve();
	cr->set_source_rgba(0, 0, 0, 0.5);
	cr->stroke();

	// ball:
	cr->set_source_rgb(1, 1, 1);
	cr->arc(offsetX+scaling*w.ball()->position().x, offsetY+scaling*w.ball()->position().y, scaling*w.ball()->radius(), 0.0, 2.0 * M_PI);
	cr->fill();

	// players:
	static const double TEAM_COLOURS[2][3] = {
		{1.0, 0.2, 0.2},
		{0.2, 0.2, 1.0}
	};
	for (unsigned int i = 0; i < 2; i++)
		for (unsigned int j = 0; j < Team::SIZE; j++) {
			PPlayer player = w.team(i)->player(j);
			cr->set_source_rgb(TEAM_COLOURS[i][0], TEAM_COLOURS[i][1], TEAM_COLOURS[i][2]);
			cr->arc(offsetX+scaling*player->position().x, offsetY+scaling*player->position().y, scaling*player->radius(), 0.0, 2.0 * M_PI);    	
			cr->fill();

			cr->move_to(offsetX+scaling*player->position().x, offsetY+scaling*player->position().y);
			cr->set_source_rgb(1.0, 1.0, 1.0);
			Vector2 angle = player->orientation();
			angle *= player->radius();
			cr->line_to(offsetX+scaling*(angle.x + player->position().x), offsetY+scaling*(angle.y + player->position().y));
			cr->stroke();

			cr->move_to(offsetX + scaling * player->position().x, offsetY + scaling * player->position().y);
			cr->set_source_rgb(1.0, 1.0, 0.0);
			cr->line_to(offsetX + scaling * (player->position().x + player->requestedVelocity().x * 10), offsetY + scaling * (player->position().y + player->requestedVelocity().y * 10));
			cr->stroke();
		}

	return true;
}

void Visualizer::update(void) {
	// Force the program to redraw the entire Visualizer.
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
		Gdk::Rectangle r(0, 0, get_allocation().get_width(), get_allocation().get_height());
		win->invalidate_rect(r, false);
	}

	// Dispatch some events.
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration(false);
}

