#ifndef UICOMPONENTS_VISUALIZER_H
#define UICOMPONENTS_VISUALIZER_H

#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include <gtkmm.h>

/**
 * A collection of data that can be visualized.
 */
class visualizable : public noncopyable {
	public:
		/**
		 * An encoding of a colour.
		 */
		class colour {
			public:
				/**
				 * The red component, from 0 to 1.
				 */
				double red;

				/**
				 * The green component, from 0 to 1.
				 */
				double green;

				/**
				 * The blue component, from 0 to 1.
				 */
				double blue;

				/**
				 * Constructs a new colour.
				 *
				 * \param red the red component
				 *
				 * \param green the green component
				 *
				 * \param blue the blue component
				 */
				colour(double red, double green, double blue) : red(red), green(green), blue(blue) {
				}
		};

		/**
		 * A collection of dimensions describing the shape of the field.
		 */
		class field : public noncopyable {
			public:
				/**
				 * \return true if the geometry data in the field is valid, or
				 * false if not
				 */
				virtual bool valid() const = 0;

				/**
				 * \return The length of the field, from goal to goal, in metres
				 */
				virtual double length() const = 0;

				/**
				 * \return The length of the field, including the boundary and referee
				 * area, in metres
				 */
				virtual double total_length() const = 0;

				/**
				 * \return The width of the field, from sideline to sideline, in metres
				 */
				virtual double width() const = 0;

				/**
				 * \return The width of the field, including the boundary and referee
				 * area, in metres
				 */
				virtual double total_width() const = 0;

				/**
				 * \return The width of the goal, symmetric above and below the
				 * centreline, from goalpost to goalpost, in metres
				 */
				virtual double goal_width() const = 0;

				/**
				 * \return The radius of the centre circle, in metres
				 */
				virtual double centre_circle_radius() const = 0;

				/**
				 * \return The radius of the arcs at the top and bottom of the defense
				 * area, in metres
				 */
				virtual double defense_area_radius() const = 0;

				/**
				 * \return The width of the straight part of the defense area between
				 * the two arcs, in metres
				 */
				virtual double defense_area_stretch() const = 0;

				/**
				 * Fired when any of the field parameters may have changed.
				 */
				mutable sigc::signal<void> signal_changed;
		};

		/**
		 * An object that may be able to be dragged by the user.
		 */
		class draggable : public byref {
			public:
				/**
				 * A pointer to a draggable.
				 */
				typedef Glib::RefPtr<draggable> ptr;

				/**
				 * \return true if this object can actually be dragged, or false
				 * if not
				 */
				virtual bool visualizer_can_drag() const = 0;

				/**
				 * Drags this object to the specified position.
				 *
				 * \param pos the position to drag to.
				 */
				virtual void visualizer_drag(const point &pos) = 0;
		};

		/**
		 * A ball that can be drawn by the visualizer.
		 */
		class ball : public draggable {
			public:
				/**
				 * A pointer to a ball.
				 */
				typedef Glib::RefPtr<ball> ptr;

				/**
				 * \return The position of the ball
				 */
				virtual point position() const = 0;
		};

		/**
		 * A robot that can be drawn by the visualizer.
		 */
		class robot : public draggable {
			public:
				/**
				 * A pointer to a robot.
				 */
				typedef Glib::RefPtr<robot> ptr;

				/**
				 * \return The position of the robot
				 */
				virtual point position() const = 0;

				/**
				 * \return The orientation of the robot
				 */
				virtual double orientation() const = 0;

				/**
				 * \return true if the robot is visible on the field, or false
				 * if not
				 */
				virtual bool visualizer_visible() const = 0;

				/**
				 * \return The colour of the robot
				 */
				virtual visualizable::colour visualizer_colour() const = 0;

				/**
				 * \return The text to display over the robot
				 */
				virtual Glib::ustring visualizer_label() const = 0;

				/**
				 * \return true if it is possible to determine the current
				 * destination of this object
				 */
				virtual bool has_destination() const = 0;

				/**
				 * \return The current destination of the robot
				 */
				virtual point destination() const = 0;
		};

		/**
		 * Fired when the robots, ball, or field have changed in some way.
		 */
		mutable sigc::signal<void> signal_visdata_changed;

		/**
		 * \return The field
		 */
		virtual const visualizable::field &field() const = 0;

		/**
		 * \return The ball
		 */
		virtual visualizable::ball::ptr ball() const = 0;

		/**
		 * \return The number of robots.
		 */
		virtual std::size_t size() const = 0;

		/**
		 * \param index the index of the robot to retreive
		 *
		 * \return The robot
		 */
		virtual visualizable::robot::ptr operator[](unsigned int index) const = 0;
};

/**
 * Displays a view of the field.
 */
class visualizer : public Gtk::DrawingArea, public noncopyable {
	public:
		/**
		 * Constructs a new visualizer.
		 *
		 * \param data the visualizable data source to display
		 */
		visualizer(const visualizable &data);

		/**
		 * Redraws the visualizer.
		 */
		void update();

	private:
		const visualizable &data;
		double scale;
		double xtranslate, ytranslate;
		sigc::connection update_connection;
		visualizable::draggable::ptr dragging;
		visualizable::draggable::ptr veldragging;

		void on_show();
		void on_hide();
		void on_size_allocate(Gtk::Allocation &);
		bool on_expose_event(GdkEventExpose *);
		bool on_button_press_event(GdkEventButton *);
		bool on_button_release_event(GdkEventButton *);
		bool on_motion_notify_event(GdkEventMotion *);
		bool on_leave_notify_event(GdkEventCrossing *);

		void compute_scales();
		double xtog(double x) __attribute__((warn_unused_result)) { return  x * scale + xtranslate; }
		double ytog(double y) __attribute__((warn_unused_result)) { return -y * scale + ytranslate; }
		double atog(double r) __attribute__((warn_unused_result)) { return -r; }
		double dtog(double d) __attribute__((warn_unused_result)) { return d * scale; }
		double xtow(double x) __attribute__((warn_unused_result)) { return  (x - xtranslate) / scale; }
		double ytow(double y) __attribute__((warn_unused_result)) { return -(y - ytranslate) / scale; }
		double atow(double r) __attribute__((warn_unused_result)) { return -r; }
		double dtow(double d) __attribute__((warn_unused_result)) { return d / scale; }
		visualizable::draggable::ptr object_at(const point &pos) const;
};

#endif

