#ifndef UICOMPONENTS_VISUALIZER_H
#define UICOMPONENTS_VISUALIZER_H

#include <gtkmm/drawingarea.h>
#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"

//
// Displays a view of the field.
//
class visualizer : public Gtk::DrawingArea {
	public:
		//
		// Constructs a new visualizer. All objects should be given for the same
		// coordinate system.
		//
		visualizer(const field &field, const ball &ball, const team::ptr west_team, const team::ptr east_team);

		//
		// Updates the display.
		//
		void update();

	protected:
		//
		// Invoked when the control is exposed.
		//
		virtual bool on_expose_event(GdkEventExpose *event) {
			update();
			return true;
		}

	private:
		const field &the_field;
		const ball &the_ball;
		const team::ptr west_team;
		const team::ptr east_team;
};

#endif

