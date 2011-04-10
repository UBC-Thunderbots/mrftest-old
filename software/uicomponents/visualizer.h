#ifndef UICOMPONENTS_VISUALIZER_H
#define UICOMPONENTS_VISUALIZER_H

#include "geom/point.h"
#include "util/byref.h"
#include <gtkmm.h>

namespace Visualizable {
	/**
	 * An encoding of the colour of a robot.
	 */
	class Colour {
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
			 * Constructs a new Colour.
			 *
			 * \param[in] red the red component, from 0 to 1.
			 *
			 * \param[in] green the green component, from 0 to 1.
			 *
			 * \param[in] blue the blue component, from 0 to 1.
			 */
			Colour(double red, double green, double blue) : red(red), green(green), blue(blue) {
			}
	};

	/**
	 * A collection of dimensions describing the shape of the field.
	 */
	class Field {
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
	 * A ball that can be drawn by the Visualizer.
	 */
	class Ball {
		public:
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

			/**
			 * Returns whether or not the ball should be highlighted.
			 *
			 * \return \c true if the ball should be highlighted, or \c false if not.
			 */
			virtual bool highlight() const = 0;

			/**
			 * Returns the colour of the highlight.
			 *
			 * \return the colour of the highlight.
			 */
			virtual Colour highlight_colour() const = 0;
	};

	/**
	 * A Robot that can be drawn by the Visualizer.
	 */
	class Robot : public virtual ByRef {
		public:
			/**
			 * A pointer to a Robot.
			 */
			typedef RefPtr<const Robot> Ptr;

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
			 * Returns the velocity of the robot.
			 *
			 * \return the velocity of the robot.
			 */
			virtual Point velocity() const = 0;

			/**
			 * Returns the colour of the robot.
			 *
			 * \return the colour of the Robot.
			 */
			virtual Colour visualizer_colour() const = 0;

			/**
			 * Returns the text to display over the robot.
			 *
			 * \return the text to display over the Robot.
			 */
			virtual Glib::ustring visualizer_label() const = 0;

			/**
			 * Returns whether or not the robot should be highlighted.
			 *
			 * \return \c true if the robot should be highlighted, or \c false if not.
			 */
			virtual bool highlight() const = 0;

			/**
			 * Returns the colour of the highlight.
			 *
			 * \return the colour of the highlight.
			 */
			virtual Colour highlight_colour() const = 0;

			/**
			 * Returns whether or not the robot has a destination.
			 *
			 * \return \c true if the robot has a destination, or \c false if not.
			 */
			virtual bool has_destination() const = 0;

			/**
			 * Returns the robot's destination.
			 *
			 * \return the destination position and orientation.
			 */
			virtual const std::pair<Point, double> &destination() const = 0;

			/**
			 * Returns whether or not the robot has a path.
			 *
			 * \return \c true if the robot has a path, or \c false if not.
			 */
			virtual bool has_path() const = 0;

			/**
			 * Returns the robot's path.
			 *
			 * \return the path the robot should follow.
			 */
			virtual const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const = 0;
	};

	/**
	 * A collection of data that can be visualized.
	 */
	class World {
		public:
			/**
			 * Returns the field.
			 *
			 * \return the field.
			 */
			virtual const Field &field() const = 0;

			/**
			 * Returns the ball.
			 *
			 * \return the ball.
			 */
			virtual const Ball &ball() const = 0;

			/**
			 * Returns the number of robots in the world.
			 *
			 * \return the number of robots.
			 */
			virtual std::size_t visualizable_num_robots() const = 0;

			/**
			 * Fetches a robot.
			 *
			 * \param[in] index the index of the Robot to retrieve.
			 *
			 * \return the Robot.
			 */
			virtual Robot::Ptr visualizable_robot(std::size_t index) const = 0;

			/**
			 * Returns the signal fired once every timer tick.
			 *
			 * \return the tick signal.
			 */
			virtual sigc::signal<void> &signal_tick() const = 0;

			/**
			 * Indicates that the mouse was pressed over the visualizer.
			 *
			 * \param[in] p the point, in world coordinates, over which the mouse was pressed.
			 *
			 * \param[in] btn the number of the button that was pressed.
			 */
			virtual void mouse_pressed(Point p, unsigned int btn) = 0;

			/**
			 * Indicates that the mouse was released over the visualizer.
			 *
			 * \param[in] p the point, in world coordinates, over which the mouse was released.
			 *
			 * \param[in] btn the number of the button that was released.
			 */
			virtual void mouse_released(Point p, unsigned int btn) = 0;

			/**
			 * Indicates that the mouse exited the area of the visualizer.
			 */
			virtual void mouse_exited() = 0;

			/**
			 * Indicates that the mouse was moved over the visualizer.
			 *
			 * \param[in] p the new position of the mouse pointer, in world coordinates.
			 */
			virtual void mouse_moved(Point p) = 0;

			/**
			 * Permits the visualizable object to draw an overlay on top of the field.
			 *
			 * \param[in] ctx the Cairo context to draw into.
			 */
			virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const = 0;
	};
}

/**
 * Displays a view of the field.
 */
class Visualizer : public Gtk::DrawingArea {
	public:
		/**
		 * Whether or not to draw the field lines
		 */
		bool show_field;

		/**
		 * Whether or not to draw the ball.
		 */
		bool show_ball;

		/**
		 * Whether or not to draw the ball's estimated velocity.
		 */
		bool show_ball_v;

		/**
		 * Whether or not to draw the robots.
		 */
		bool show_robots;

		/**
		 * Whether or not to draw the robots' estimated velocities.
		 */
		bool show_robots_v;

		/**
		 * Whether or not to draw the robots' AI-assigned destinations.
		 */
		bool show_robots_dest;

		/**
		 * Whether or not to draw the robots' navigator-assigned paths.
		 */
		bool show_robots_path;

		/**
		 * Whether or not to draw the AI overlay.
		 */
		bool show_overlay;

		/**
		 * Constructs a new Visualizer.
		 *
		 * \param[in] data the Visualizable data source to display.
		 */
		Visualizer(Visualizable::World &data);

		/**
		 * Redraws the Visualizer.
		 */
		void update();

		/**
		 * Returns the signal fired when the mouse is moved over the visualizer.
		 *
		 * \return the mouse motion signal.
		 */
		sigc::signal<void, Point> &signal_mouse_moved() const;

	private:
		Visualizable::World &data;
		double scale;
		double xtranslate, ytranslate;
		sigc::connection update_connection;
		mutable sigc::signal<void, Point> signal_mouse_moved_;

		void on_show();
		void on_hide();
		void on_size_allocate(Gtk::Allocation &);
		bool on_expose_event(GdkEventExpose *);
		bool on_button_press_event(GdkEventButton *evt);
		bool on_button_release_event(GdkEventButton *evt);
		bool on_motion_notify_event(GdkEventMotion *evt);
		bool on_leave_notify_event(GdkEventCrossing *evt);

		void compute_scales();
		double xtow(double x) __attribute__((warn_unused_result)) { return (x - xtranslate) / scale; }
		double ytow(double y) __attribute__((warn_unused_result)) { return -(y - ytranslate) / scale; }
		double atow(double r) __attribute__((warn_unused_result)) { return -r; }
		double dtow(double d) __attribute__((warn_unused_result)) { return d / scale; }
};

#endif

