#include "geom/angle.h"
#include "robot_controller/joystick/joystick.h"
#include "robot_controller/robot_controller.h"
#include <gtkmm.h>

namespace {
	const double MAX_LINEAR_VELOCITY = 1.0;
	const double MAX_ANGULAR_VELOCITY = PI;
	const unsigned int AXIS_FB = 4; // Right stick Y axis.
	const unsigned int AXIS_LR = 3; // Right stick X axis.
	const unsigned int AXIS_ROT = 0; // Left stick X axis.

	class joystick_controller_factory : public virtual robot_controller_factory {
		public:
			joystick_controller_factory() : robot_controller_factory("Joystick") {
			}

			virtual robot_controller::ptr create_controller(const Glib::ustring &robot_name);
	};

	joystick_controller_factory factory;

	class joystick_display_rectangle : public virtual Gtk::DrawingArea {
		public:
			joystick_display_rectangle() {
				set_size_request(128, 128);
			}

			void set_stick(joystick::ptr p) {
				stick = p;
			}

			void update() {
				get_window()->invalidate(false);
			}

		protected:
			virtual bool on_expose_event(GdkEventExpose *) {
				int width, height;
				get_window()->get_size(width, height);
				if (width > height) width = height;
				if (height > width) height = width;
				Cairo::RefPtr<Cairo::Context> ctx = get_window()->create_cairo_context();

				// Fill the background with black.
				ctx->set_source_rgb(0.0, 0.0, 0.0);
				ctx->move_to(0.0, 0.0);
				ctx->line_to(width, 0.0);
				ctx->line_to(width, height);
				ctx->line_to(0.0, height);
				ctx->fill();

				// Draw the reticle.
				ctx->set_source_rgb(0.25, 0.25, 0.25);
				ctx->move_to(width / 2.0, 0.0);
				ctx->line_to(width / 2.0, height);
				ctx->stroke();
				ctx->move_to(0.0, height / 2.0);
				ctx->line_to(width, height / 2.0);
				ctx->stroke();

				if (stick) {
					// Draw the angular velocity line.
					double t = stick->axis(AXIS_ROT);
					t = t / 32767.0 * PI;
					const point &rot = point(0.0, 1.0).rotate(t) * width / 2.0;
					ctx->set_source_rgb(0.0, 1.0, 0.0);
					ctx->move_to(width / 2.0, height / 2.0);
					ctx->line_to(rot.x + width / 2.0, rot.y + height / 2.0);
					ctx->stroke();

					// Draw the linear velocity point.
					double x = stick->axis(AXIS_LR);
					double y = stick->axis(AXIS_FB);
					x = (x / 32767.0 / 2.0 + 0.5) * width;
					y = (y / 32767.0 / 2.0 + 0.5) * height;
					ctx->set_source_rgb(1.0, 0.0, 0.0);
					ctx->arc(x, y, 1.0, 0.0, 2.0 * PI);
				}

				return true;
			}

		private:
			joystick::ptr stick;
	};

	class joystick_controller : public virtual robot_controller, public virtual Gtk::VBox {
		public:
			joystick_controller(const Glib::ustring &robot_name);

			virtual ~joystick_controller();

			virtual void move(const point &, const point &, double, double, point &linear_velocity, double &angular_velocity) {
				if (stick) {
					linear_velocity.x = -stick->axis(AXIS_FB) / 32767.0 * MAX_LINEAR_VELOCITY;
					linear_velocity.y = -stick->axis(AXIS_LR) / 32767.0 * MAX_LINEAR_VELOCITY;
					angular_velocity = -stick->axis(AXIS_ROT) / 32767.0 * MAX_ANGULAR_VELOCITY;
				} else {
					linear_velocity.x = 0;
					linear_velocity.y = 0;
					angular_velocity = 0;
				}
				disp.update();
			}

			virtual robot_controller_factory &get_factory() const {
				return factory;
			}

		private:
			joystick::ptr stick;

			Gtk::ComboBoxText joybox;

			joystick_display_rectangle disp;

			void joy_changed() {
				int idx = joybox.get_active_row_number();
				if (idx > 0) {
					stick = joystick::create(joystick::list()[idx - 1].first);
				} else {
					stick.reset();
				}
				disp.set_stick(stick);
			}
	};

	robot_controller::ptr joystick_controller_factory::create_controller(const Glib::ustring &robot_name) {
		robot_controller::ptr p(new joystick_controller(robot_name));
		return p;
	}

	class joystick_controller_ui : public virtual Gtk::Window {
		public:
			joystick_controller_ui() {
				add(book);
			}

			void add_controller(joystick_controller &ctl, const Glib::ustring &title) {
				book.append_page(ctl, title);
				show_all();
			}

			void remove_controller(joystick_controller &ctl) {
				book.remove_page(ctl);
				if (!book.get_n_pages())
					hide_all();
			}

		protected:
			virtual bool on_delete_event(GdkEventAny *) {
				Gtk::Main::quit();
				return true;
			}

		private:
			Gtk::Notebook book;
	};

	joystick_controller_ui &get_ui() {
		static joystick_controller_ui *ui = 0;
		if (!ui)
			ui = new joystick_controller_ui();
		return *ui;
	}

	joystick_controller::joystick_controller(const Glib::ustring &robot_name) {
		joybox.append_text("<Choose Joystick>");
		const std::vector<std::pair<Glib::ustring, Glib::ustring> > &sticks = joystick::list();
		for (unsigned int i = 0; i < sticks.size(); i++)
			joybox.append_text(Glib::ustring::compose("%1 [%2]", sticks[i].second, sticks[i].first));
		joybox.set_active_text("<Choose Joystick>");
		joybox.signal_changed().connect(sigc::mem_fun(*this, &joystick_controller::joy_changed));
		pack_start(joybox, false, false);

		pack_start(disp, true, true);

		get_ui().add_controller(*this, robot_name);
	}

	joystick_controller::~joystick_controller() {
		get_ui().remove_controller(*this);
	}
}

