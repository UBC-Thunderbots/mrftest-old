#ifndef UICOMPONENTS_VISUALIZER_H
#define UICOMPONENTS_VISUALIZER_H

#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include <gtkmm.h>

/**
 * A collection of data that can be visualized.
 */
class Visualizable : public NonCopyable {
	public:
		/**
		 * An encoding of the colour of a robot.
		 */
		class RobotColour {
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
				 * Constructs a new RobotColour.
				 *
				 * \param[in] red the red component, from 0 to 1.
				 *
				 * \param[in] green the green component, from 0 to 1.
				 *
				 * \param[in] blue the blue component, from 0 to 1.
				 */
				RobotColour(double red, double green, double blue) : red(red), green(green), blue(blue) {
				}
		};

		/**
		 * A collection of dimensions describing the shape of the field.
		 */
		class Field : public NonCopyable {
			public:
				/**
				 * Checks whether the data is valid.
				 *
				 * \return true if the geometry data in the Field is valid, or false if not.
				 */
				virtual bool valid() const = 0;

				/**
				 * Returns the length of the field.
				 *
				 * \return the length of the field, from goal to goal, in metres.
				 */
				virtual double length() const = 0;

				/**
				 * Returns the total length of the field.
				 *
				 * \return the length of the field, including the boundary and referee area, in metres.
				 */
				virtual double total_length() const = 0;

				/**
				 * Returns the width of the field.
				 *
				 * \return the width of the field, from sideline to sideline, in metres.
				 */
				virtual double width() const = 0;

				/**
				 * Returns the total width of the field.
				 *
				 * \return the width of the field, including the boundary and referee area, in metres.
				 */
				virtual double total_width() const = 0;

				/**
				 * Returns the width of the goal.
				 *
				 * \return the width of the goal, symmetric above and below the centreline, from goalpost to goalpost, in metres.
				 */
				virtual double goal_width() const = 0;

				/**
				 * Returns the radius of the centre circle.
				 *
				 * \return the radius of the centre circle, in metres.
				 */
				virtual double centre_circle_radius() const = 0;

				/**
				 * Returns the radius of the arcs at the top and bottom of the
				 * defense area.
				 *
				 * \return the radius of the arcs at the top and bottom of the defense area, in metres.
				 */
				virtual double defense_area_radius() const = 0;

				/**
				 * Returns the width of the straight part of the defense area.
				 *
				 * \return the width of the straight part of the defense area between the two arcs, in metres.
				 */
				virtual double defense_area_stretch() const = 0;

				/**
				 * Fired when any of the Field parameters may have changed.
				 */
				mutable sigc::signal<void> signal_changed;
		};

		/**
		 * An object that may be able to be dragged by the user.
		 */
		class Draggable : public ByRef {
			public:
				/**
				 * A pointer to a Draggable.
				 */
				typedef RefPtr<Draggable> Ptr;

				/**
				 * Checks if the object can actually be dragged.
				 *
				 * \return true if this object can actually be dragged, or false if not.
				 */
				virtual bool visualizer_can_drag() const = 0;

				/**
				 * Drags this object to the specified position.
				 *
				 * \param[in] pos the position to drag to.
				 */
				virtual void visualizer_drag(const Point &pos) = 0;
		};

		/**
		 * A ball that can be drawn by the Visualizer.
		 */
		class Ball : public Draggable {
			public:
				/**
				 * A pointer to a Ball.
				 */
				typedef RefPtr<Ball> Ptr;

				/**
				 * Returns the position of the ball.
				 *
				 * \return the position of the ball.
				 */
				virtual Point position() const = 0;

				/**
				 * Returns the velocity of the ball.
				 *
				 * \return the velocity of the ball.
				 */
				virtual Point velocity() const = 0;
		};

		/**
		 * A Robot that can be drawn by the Visualizer.
		 */
		class Robot : public Draggable {
			public:
				/**
				 * A pointer to a Robot.
				 */
				typedef RefPtr<Robot> Ptr;

				/**
				 * Returns the position of the robot.
				 *
				 * \return the position of the Robot.
				 */
				virtual Point position() const = 0;

				/**
				 * Returns the orientation of the robot.
				 *
				 * \return the orientation of the Robot.
				 */
				virtual double orientation() const = 0;

				/**
				 * Checks whether the robot is visible.
				 *
				 * \return true if the Robot is visible on the field, or false if not.
				 */
				virtual bool visualizer_visible() const = 0;

				/**
				 * Returns the colour of the robot.
				 *
				 * \return the colour of the Robot.
				 */
				virtual Visualizable::RobotColour visualizer_colour() const = 0;

				/**
				 * Returns the text to display over the robot.
				 *
				 * \return the text to display over the Robot.
				 */
				virtual Glib::ustring visualizer_label() const = 0;

				/**
				 * Checks whether the robot has a definite destination.
				 *
				 * \return true if it is possible to determine the current destination of this object.
				 */
				virtual bool has_destination() const = 0;

				/**
				 * Returns the current destination of the robot.
				 *
				 * \return the current destination of the Robot.
				 */
				virtual Point destination() const = 0;
		};

		/**
		 * Fired when the robots, ball, or field have changed in some way.
		 */
		mutable sigc::signal<void> signal_visdata_changed;

		/**
		 * Returns the field.
		 *
		 * \return the field.
		 */
		virtual const Visualizable::Field &field() const = 0;

		/**
		 * Returns the ball.
		 *
		 * \return the ball.
		 */
		virtual Visualizable::Ball::Ptr ball() const = 0;

		/**
		 * Returns the number of robots in the world.
		 *
		 * \return the number of robots.
		 */
		virtual std::size_t size() const = 0;

		/**
		 * Fetches a robot.
		 *
		 * \param[in] index the index of the Robot to retrieve.
		 *
		 * \return the Robot.
		 */
		virtual Visualizable::Robot::Ptr operator[](unsigned int index) const = 0;
};

/**
 * Displays a view of the field.
 */
class Visualizer : public Gtk::DrawingArea, public NonCopyable {
	public:
		/**
		 * Constructs a new Visualizer.
		 *
		 * \param[in] data the Visualizable data source to display.
		 */
		Visualizer(const Visualizable &data);

		/**
		 * Redraws the Visualizer.
		 */
		void update();

		/**
		 * Returns the overlay context.
		 *
		 * \return a context for drawing on the overlay surface that renders on top of the Visualizer.
		 */
		Cairo::RefPtr<Cairo::Context> overlay() const;

		/**
		 * Fired every time the overlay surface changes.
		 */
		mutable sigc::signal<void> signal_overlay_changed;

	private:
		const Visualizable &data;
		double scale;
		double xtranslate, ytranslate;
		sigc::connection update_connection;
		Visualizable::Draggable::Ptr dragging;
		Visualizable::Draggable::Ptr veldragging;
		Cairo::RefPtr<Cairo::ImageSurface> overlay_;

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
		Visualizable::Draggable::Ptr object_at(const Point &pos) const;
};

#endif

