#ifndef UICOMPONENTS_VISUALIZER_H
#define UICOMPONENTS_VISUALIZER_H

#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"
#include <gtkmm.h>

//
// Displays a view of the field.
//
class visualizer : public Gtk::DrawingArea {
	public:
		//
		// Constructs a new visualizer. All objects should be given for the same
		// coordinate system.
		//
		visualizer(const field::ptr field, const ball::ptr ball, const team::ptr west_team, const team::ptr east_team);

		//
		// Updates the display.
		//
		void update();

	protected:
		void on_size_allocate(Gtk::Allocation &);
		bool on_expose_event(GdkEventExpose *);
		bool on_button_press_event(GdkEventButton *);
		bool on_button_release_event(GdkEventButton *);
		bool on_motion_notify_event(GdkEventMotion *);
		bool on_leave_notify_event(GdkEventCrossing *);

	private:
		double scale;
		double xtranslate, ytranslate;
		const field::ptr the_field;
		const ball::ptr the_ball;
		const team::ptr west_team;
		const team::ptr east_team;
		draggable::ptr dragging;

		double xtog(double x) { return  x * scale + xtranslate; }
		double ytog(double y) { return -y * scale + ytranslate; }
		double atog(double r) { return -r; }
		double dtog(double d) { return d * scale; }
		double xtow(double x) { return  (x - xtranslate) / scale; }
		double ytow(double y) { return -(y - ytranslate) / scale; }
		double atow(double r) { return -r; }
		double dtow(double d) { return d / scale; }
};

#endif

