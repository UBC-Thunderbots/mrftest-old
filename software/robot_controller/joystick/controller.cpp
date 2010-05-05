#include "geom/angle.h"
#include "robot_controller/joystick/joystick.h"
#include "robot_controller/robot_controller.h"
#include "world/player_impl.h"
#include <gtkmm.h>

namespace {
	const double MAX_LINEAR_VELOCITY = 1.0;
	const double MAX_ANGULAR_VELOCITY = PI;
	const unsigned int AXIS_FB = 4; // Right stick Y axis.
	const unsigned int AXIS_LR = 3; // Right stick X axis.
	const unsigned int AXIS_ROT = 0; // Left stick X axis.
	const unsigned int AXIS_DRIBBLE = 2; // Left trigger.
	const unsigned int AXIS_CHICK_POWER = 5; // Right trigger.
	const unsigned int BTN_KICK = 0; // A.
	const unsigned int BTN_CHIP = 2; // X.

	class joystick_controller_factory : public robot_controller_factory {
		public:
			joystick_controller_factory() : robot_controller_factory("Joystick") {
			}

			robot_controller::ptr create_controller(player_impl::ptr plr, bool yellow, unsigned int index) const;
	};

	joystick_controller_factory factory;

	class joystick_display_rectangle : public Gtk::DrawingArea {
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
			bool on_expose_event(GdkEventExpose *) {
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
					const point &rot = point(0.0, -1.0).rotate(t) * width / 2.0;
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
					ctx->fill();
				}

				return true;
			}

		private:
			joystick::ptr stick;
	};

	class joystick_controller : public robot_controller, public Gtk::VBox {
		public:
			joystick_controller(player_impl::ptr plr, bool yellow, unsigned int index);

			~joystick_controller();

			void move(const point &, double, point &linear_velocity, double &angular_velocity);

			robot_controller_factory &get_factory() const {
				return factory;
			}

		private:
			player_impl::ptr plr;
			joystick::ptr stick;
			Gtk::ComboBoxText joybox;
			joystick_display_rectangle disp;
			sigc::connection move_connection;
			bool prev_chick;

			void joy_changed() {
				move_connection.disconnect();
				int idx = joybox.get_active_row_number();
				if (idx > 0) {
					stick = joystick::create(joystick::list()[idx - 1].first);
				} else {
					stick.reset();
				}
				disp.set_stick(stick);
				move_connection = stick->signal_moved().connect(sigc::mem_fun(disp, &joystick_display_rectangle::update));
			}
	};

	robot_controller::ptr joystick_controller_factory::create_controller(player_impl::ptr plr, bool yellow, unsigned int index) const {
		robot_controller::ptr p(new joystick_controller(plr, yellow, index));
		return p;
	}

	class joystick_controller_ui : public Gtk::Window {
		public:
			joystick_controller_ui() : abs_check("Absolute") {
				set_title("Joystick Configuration");
				vbox.pack_start(abs_check, false, false);
				vbox.pack_start(book, true, true);
				add(vbox);
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

			bool absolute() {
				return abs_check.get_active();
			}

		protected:
			bool on_delete_event(GdkEventAny *) {
				Gtk::Main::quit();
				return true;
			}

		private:
			Gtk::VBox vbox;
			Gtk::CheckButton abs_check;
			Gtk::Notebook book;
	};

	joystick_controller_ui &get_ui() {
		static joystick_controller_ui ui;
		return ui;
	}

	joystick_controller::joystick_controller(player_impl::ptr plr, bool yellow, unsigned int index) : plr(plr), prev_chick(false) {
		joybox.append_text("<Choose Joystick>");
		const std::vector<std::pair<Glib::ustring, Glib::ustring> > &sticks = joystick::list();
		for (unsigned int i = 0; i < sticks.size(); i++)
			joybox.append_text(Glib::ustring::compose("%1 [%2]", sticks[i].second, sticks[i].first));
		joybox.set_active_text("<Choose Joystick>");
		joybox.signal_changed().connect(sigc::mem_fun(this, &joystick_controller::joy_changed));
		pack_start(joybox, false, false);

		pack_start(disp, true, true);

		get_ui().add_controller(*this, Glib::ustring::compose("%1 %2", yellow ? "Yellow" : "Blue", index));
	}

	joystick_controller::~joystick_controller() {
		get_ui().remove_controller(*this);
	}

	void joystick_controller::move(const point &, double, point &linear_velocity, double &angular_velocity) {
		if (stick) {
			if (get_ui().absolute()) {
				// World coordinates.
				// Positive world X is east.
				// Positive world Y is north.
				linear_velocity.x =  stick->axis(AXIS_LR) / 32767.0 * MAX_LINEAR_VELOCITY;
				linear_velocity.y = -stick->axis(AXIS_FB) / 32767.0 * MAX_LINEAR_VELOCITY;
				linear_velocity = linear_velocity.rotate(-plr->orientation());
			} else {
				// Robot coordinates:
				// Positive robot X is forward.
				// Positive robot Y is left.
				linear_velocity.x = -stick->axis(AXIS_FB) / 32767.0 * MAX_LINEAR_VELOCITY;
				linear_velocity.y = -stick->axis(AXIS_LR) / 32767.0 * MAX_LINEAR_VELOCITY;
			}
			angular_velocity = -stick->axis(AXIS_ROT) / 32767.0 * MAX_ANGULAR_VELOCITY;
			plr->dribble((stick->axis(AXIS_DRIBBLE) + 32767) / 65535.0);
			double chick_power = (stick->axis(AXIS_CHICK_POWER) + 32767) / 65535.0;
			if (stick->button(BTN_KICK) && !prev_chick) {
				plr->kick(chick_power);
				prev_chick = true;
			} else if (stick->button(BTN_CHIP) && !prev_chick) {
				plr->chip(chick_power);
				prev_chick = true;
			} else if (!stick->button(BTN_KICK) && !stick->button(BTN_CHIP)) {
				prev_chick = false;
			}
		} else {
			linear_velocity.x = 0;
			linear_velocity.y = 0;
			angular_velocity = 0;
			plr->dribble(0);
			prev_chick = false;
		}
	}
}

