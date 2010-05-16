#if 0
#ifndef UICOMPONENTS_VISUALIZER_H
#define UICOMPONENTS_VISUALIZER_H

#include "util/noncopyable.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"
#include <gtkmm.h>

//
// Displays a view of the field.
//
class visualizer : public Gtk::DrawingArea, public noncopyable {
	public:
		//
		// Constructs a new visualizer. All objects should be given for the same
		// coordinate system.
		//
		visualizer(const field::ptr field, const ball::ptr ball, const team::ptr west_team, const team::ptr east_team, bool draggable);

		//
		// Redraws the visualizer. This function needs to be called after each
		// change to the state of the world.
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
		bool draggable;
		double scale;
		double xtranslate, ytranslate;
		const field::ptr the_field;
		const ball::ptr the_ball;
		const team::ptr west_team;
		const team::ptr east_team;
		draggable::ptr dragging, veldragging;
		point dragged_velocity;

		double xtog(double x) __attribute__((warn_unused_result)) { return  x * scale + xtranslate; }
		double ytog(double y) __attribute__((warn_unused_result)) { return -y * scale + ytranslate; }
		double atog(double r) __attribute__((warn_unused_result)) { return -r; }
		double dtog(double d) __attribute__((warn_unused_result)) { return d * scale; }
		double xtow(double x) __attribute__((warn_unused_result)) { return  (x - xtranslate) / scale; }
		double ytow(double y) __attribute__((warn_unused_result)) { return -(y - ytranslate) / scale; }
		double atow(double r) __attribute__((warn_unused_result)) { return -r; }
		double dtow(double d) __attribute__((warn_unused_result)) { return d / scale; }
		draggable::ptr object_at(const point &pos) const;
};

#endif
#else
#warning IMPLEMENT THIS
#endif

