#include "ai/world/player.h"
#include "robot_controller/joystick/joystick.h"
#include "robot_controller/robot_controller.h"
#include <gtkmm.h>

namespace {
	const double MAX_LINEAR_VELOCITY = 3.0;
	const double MAX_ANGULAR_VELOCITY = 5.0 * M_PI;
	const unsigned int AXIS_FB = 4; // Right stick Y axis.
	const unsigned int AXIS_LR = 3; // Right stick X axis.
	const unsigned int AXIS_ROT = 0; // Left stick X axis.
	const unsigned int AXIS_DRIBBLE = 2; // Left trigger.
	const unsigned int AXIS_CHICK_POWER = 5; // Right trigger.
	const unsigned int BTN_KICK = 0; // A.
	const unsigned int BTN_CHIP = 2; // X.

	class JoystickControllerFactory : public RobotControllerFactory {
		public:
			JoystickControllerFactory() : RobotControllerFactory("Joystick") {
			}

			RobotController::ptr create_controller(Player::ptr plr, bool yellow, unsigned int index) const;
	};

	JoystickControllerFactory factory;

	class JoystickDisplayRectangle : public Gtk::DrawingArea {
		public:
			JoystickDisplayRectangle() {
				set_size_request(128, 128);
			}

			void set_stick(Joystick::ptr p) {
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
					t = t / 32767.0 * M_PI;
					const Point &rot = Point(0.0, -1.0).rotate(t) * width / 2.0;
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
					ctx->arc(x, y, 1.0, 0.0, 2.0 * M_PI);
					ctx->fill();
				}

				return true;
			}

		private:
			Joystick::ptr stick;
	};

	class JoystickController : public RobotController, public Gtk::VBox {
		public:
#warning figure out a better UI so we can avoid passing colour and index into a robot controller
			JoystickController(Player::ptr plr, bool yellow, unsigned int index);

			~JoystickController();

			void move(const Point &, double, Point &linear_velocity, double &angular_velocity);

			void clear() {
			}

			RobotControllerFactory &get_factory() const {
				return factory;
			}

		private:
			Player::ptr plr;
			Joystick::ptr stick;
			Gtk::ComboBoxText joybox;
			JoystickDisplayRectangle disp;
			sigc::connection move_connection;
			bool prev_chick;

			void joy_changed() {
				move_connection.disconnect();
				int idx = joybox.get_active_row_number();
				if (idx > 0) {
					stick = Joystick::create(Joystick::list()[idx - 1].first);
				} else {
					stick.reset();
				}
				disp.set_stick(stick);
				move_connection = stick->signal_moved().connect(sigc::mem_fun(disp, &JoystickDisplayRectangle::update));
			}
	};

	RobotController::ptr JoystickControllerFactory::create_controller(Player::ptr plr, bool yellow, unsigned int index) const {
		RobotController::ptr p(new JoystickController(plr, yellow, index));
		return p;
	}

	class JoystickControllerUI : public Gtk::Window {
		public:
			JoystickControllerUI() : abs_check("Absolute") {
				set_title("Joystick Configuration");
				vbox.pack_start(abs_check, false, false);
				vbox.pack_start(book, true, true);
				add(vbox);
			}

			void add_controller(JoystickController &ctl, const Glib::ustring &title) {
				book.append_page(ctl, title);
				show_all();
			}

			void remove_controller(JoystickController &ctl) {
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

	JoystickControllerUI &get_ui() {
		static JoystickControllerUI ui;
		return ui;
	}

	JoystickController::JoystickController(Player::ptr plr, bool yellow, unsigned int index) : plr(plr), prev_chick(false) {
		joybox.append_text("<Choose Joystick>");
		const std::vector<std::pair<Glib::ustring, Glib::ustring> > &sticks = Joystick::list();
		for (unsigned int i = 0; i < sticks.size(); i++)
			joybox.append_text(Glib::ustring::compose("%1 [%2]", sticks[i].second, sticks[i].first));
		joybox.set_active_text("<Choose Joystick>");
		joybox.signal_changed().connect(sigc::mem_fun(this, &JoystickController::joy_changed));
		pack_start(joybox, false, false);

		pack_start(disp, true, true);

		get_ui().add_controller(*this, Glib::ustring::compose("%1 %2", yellow ? "Yellow" : "Blue", index));
	}

	JoystickController::~JoystickController() {
		get_ui().remove_controller(*this);
	}

	void JoystickController::move(const Point &, double, Point &linear_velocity, double &angular_velocity) {
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

