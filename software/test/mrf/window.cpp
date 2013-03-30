#include "test/common/mapping.h"
#include "test/mrf/window.h"
#include "uicomponents/abstract_list_model.h"
#include "util/config.h"
#include "util/joystick.h"
#include <cmath>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <gtkmm/main.h>
#include <gtkmm/treemodelcolumn.h>

class TesterWindow::MappedJoysticksModel : public Glib::Object, public AbstractListModel {
	public:
		Gtk::TreeModelColumn<Glib::ustring> node_column, name_column;

		static Glib::RefPtr<MappedJoysticksModel> create() {
			Glib::RefPtr<MappedJoysticksModel> p(new MappedJoysticksModel);
			return p;
		}

		std::size_t alm_rows() const {
			return sticks.size() + 1;
		}

		void alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const {
			if (col == static_cast<unsigned int>(node_column.index()) || col == static_cast<unsigned int>(name_column.index())) {
				Glib::Value<Glib::ustring> v;
				v.init(node_column.type());
				if (row == 0) {
					v.set(col == static_cast<unsigned int>(node_column.index()) ? u8"<None>" : u8"");
				} else {
					v.set(col == static_cast<unsigned int>(node_column.index()) ? Glib::filename_to_utf8(sticks[row - 1]->node()) : sticks[row - 1]->name());
				}
				value.init(node_column.type());
				value = v;
			} else {
				std::abort();
			}
		}

		void alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &) {
			std::abort();
		}

		const Joystick *get_device(std::size_t index) {
			return index > 0 ? sticks[index - 1] : 0;
		}

		const JoystickMapping &get_mapping(const Joystick &stick) {
			return mappings.find(stick.name().collate_key())->second;
		}

	private:
		std::vector<const Joystick *> sticks;

		std::unordered_map<std::string, JoystickMapping> mappings;

		MappedJoysticksModel() : Glib::ObjectBase(typeid(MappedJoysticksModel)) {
			alm_column_record.add(node_column);
			alm_column_record.add(name_column);

			const xmlpp::Element *joysticks_elt = Config::joysticks();
			const xmlpp::Node::NodeList &joystick_elts = joysticks_elt->get_children();
			for (auto i = joystick_elts.begin(), iend = joystick_elts.end(); i != iend; ++i) {
				const xmlpp::Node *n = *i;
				const xmlpp::Element *e = dynamic_cast<const xmlpp::Element *>(n);
				if (e) {
					if (e->get_name() != u8"joystick") {
						throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose(u8"Malformed config.xml (expected element of type joystick, found %1)", e->get_name())));
					}
					JoystickMapping m(e);
					const std::string &ck = m.name().collate_key();
					if (mappings.count(ck)) {
						throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose(u8"Malformed config.xml (duplicate joystick type %1)", m.name())));
					}
					mappings.insert(std::make_pair(ck, m));
				}
			}

			for (std::size_t i = 0; i < Joystick::count(); ++i) {
				const Joystick &stick = Joystick::get(i);
				if (mappings.count(stick.name().collate_key())) {
					sticks.push_back(&stick);
				}
			}
		}
};

TesterWindow::TesterWindow(MRFDongle &dongle, MRFRobot &robot) : mapped_joysticks(MappedJoysticksModel::create()), robot(robot), manual_commutation_window(dongle, robot.index), feedback_frame(u8"Feedback"), feedback_panel(dongle, robot), drive_frame(u8"Drive"), drive_panel(robot, &manual_commutation_window), dribble_button(u8"Dribble"), kicker_frame(u8"Kicker"), kicker_panel(robot), leds_frame(u8"LEDs"), leds_panel(dongle, robot.index), params_frame(u8"Parameters"), params_panel(dongle, robot), power_frame(u8"Power"), power_panel(dongle, robot.index), joystick_frame(u8"Joystick"), joystick_sensitivity_high_button(joystick_sensitivity_group, u8"High Sensitivity"), joystick_sensitivity_low_button(joystick_sensitivity_group, u8"Low Sensitivity"), joystick_chooser(Glib::RefPtr<Gtk::TreeModel>::cast_static(mapped_joysticks)) {
	set_title(Glib::ustring::compose(u8"Tester (%1)", robot.index));

	feedback_frame.add(feedback_panel);
	vbox1.pack_start(feedback_frame, Gtk::PACK_SHRINK);

	drive_frame.add(drive_panel);
	vbox1.pack_start(drive_frame, Gtk::PACK_SHRINK);

	dribble_button.signal_toggled().connect(sigc::mem_fun(this, &TesterWindow::on_dribble_toggled));
	vbox1.pack_start(dribble_button, Gtk::PACK_SHRINK);

	hbox.pack_start(vbox1, Gtk::PACK_EXPAND_WIDGET);

	kicker_frame.add(kicker_panel);
	vbox2.pack_start(kicker_frame, Gtk::PACK_SHRINK);

	leds_frame.add(leds_panel);
	vbox2.pack_start(leds_frame, Gtk::PACK_SHRINK);

	params_frame.add(params_panel);
	vbox2.pack_start(params_frame, Gtk::PACK_SHRINK);

	power_frame.add(power_panel);
	vbox2.pack_start(power_frame, Gtk::PACK_SHRINK);

	hbox.pack_start(vbox2, Gtk::PACK_EXPAND_WIDGET);

	outer_vbox.pack_start(hbox, Gtk::PACK_SHRINK);

	joystick_sensitivity_hbox.pack_start(joystick_sensitivity_high_button, Gtk::PACK_SHRINK);
	joystick_sensitivity_hbox.pack_start(joystick_sensitivity_low_button, Gtk::PACK_SHRINK);
	joystick_vbox.pack_start(joystick_sensitivity_hbox, Gtk::PACK_SHRINK);
	joystick_chooser.pack_start(mapped_joysticks->node_column);
	joystick_chooser.pack_start(mapped_joysticks->name_column);
	joystick_chooser.set_active(0);
	joystick_chooser.signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_chooser_changed));
	joystick_vbox.pack_start(joystick_chooser, Gtk::PACK_SHRINK);
	joystick_frame.add(joystick_vbox);
	outer_vbox.pack_start(joystick_frame, Gtk::PACK_SHRINK);

	add(outer_vbox);

	Gtk::Main::signal_key_snooper().connect(sigc::mem_fun(this, &TesterWindow::key_snoop));

	show_all();
}

TesterWindow::~TesterWindow() = default;

void TesterWindow::scram() {
	drive_panel.coast();
	dribble_button.set_active(false);
	kicker_panel.scram();
}

int TesterWindow::key_snoop(Widget *, GdkEventKey *event) {
	if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Z || event->keyval == GDK_z)) {
		// Z letter sets all controls to zero.
		drive_panel.zero();
		dribble_button.set_active(false);
	}
	return 0;
}

void TesterWindow::on_dribble_toggled() {
	robot.dribble(dribble_button.get_active());
}

void TesterWindow::on_joystick_chooser_changed() {
	std::for_each(joystick_signal_connections.begin(), joystick_signal_connections.end(), [](sigc::connection &conn) { conn.disconnect(); });
	joystick_signal_connections.clear();
	const Joystick *pstick = mapped_joysticks->get_device(static_cast<std::size_t>(joystick_chooser.get_active_row_number()));
	if (pstick) {
		const Joystick &stick = *pstick;
		const JoystickMapping &m = mapped_joysticks->get_mapping(stick);
		for (unsigned int i = 0; i < JoystickMapping::N_AXES; ++i) {
			if (m.has_axis(i)) {
				joystick_signal_connections.push_back(stick.axes()[m.axis(i)].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_drive_axis_changed)));
			}
		}
		if (m.has_button(JoystickMapping::BUTTON_DRIBBLE)) {
			joystick_signal_connections.push_back(stick.buttons()[m.button(JoystickMapping::BUTTON_DRIBBLE)].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_dribble_changed)));
		}
		if (m.has_button(JoystickMapping::BUTTON_KICK)) {
			joystick_signal_connections.push_back(stick.buttons()[m.button(JoystickMapping::BUTTON_KICK)].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_kick_changed)));
		}
		if (m.has_button(JoystickMapping::BUTTON_SCRAM)) {
			joystick_signal_connections.push_back(stick.buttons()[m.button(JoystickMapping::BUTTON_SCRAM)].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_scram_changed)));
		}
		on_joystick_drive_axis_changed();
	}
}

void TesterWindow::on_joystick_drive_axis_changed() {
	const Joystick &stick = *mapped_joysticks->get_device(static_cast<std::size_t>(joystick_chooser.get_active_row_number()));
	const JoystickMapping &m = mapped_joysticks->get_mapping(stick);
	double drive_axes[4];
	static_assert(JoystickMapping::N_AXES >= G_N_ELEMENTS(drive_axes), u8"Not enough joystick axes for drive wheels");
	for (unsigned int i = 0; i < G_N_ELEMENTS(drive_axes); ++i) {
		if (m.has_axis(i)) {
			drive_axes[i] = std::pow(-stick.axes()[m.axis(i)], 3);
		} else {
			drive_axes[i] = 0;
		}
	}
	if (joystick_sensitivity_low_button.get_active()) {
		double scale[4];
		drive_panel.get_low_sensitivity_scale_factors(scale);
		for (unsigned int i = 0; i < G_N_ELEMENTS(drive_axes); ++i) {
			drive_axes[i] *= scale[i];
		}
	}
	drive_panel.set_values(drive_axes);
}

void TesterWindow::on_joystick_dribble_changed() {
	const Joystick &stick = *mapped_joysticks->get_device(static_cast<std::size_t>(joystick_chooser.get_active_row_number()));
	const JoystickMapping &m = mapped_joysticks->get_mapping(stick);
	if (m.has_button(JoystickMapping::BUTTON_DRIBBLE) && stick.buttons()[m.button(JoystickMapping::BUTTON_DRIBBLE)]) {
		dribble_button.set_active(!dribble_button.get_active());
	}
}

void TesterWindow::on_joystick_kick_changed() {
	const Joystick &stick = *mapped_joysticks->get_device(static_cast<std::size_t>(joystick_chooser.get_active_row_number()));
	const JoystickMapping &m = mapped_joysticks->get_mapping(stick);
	if (m.has_button(JoystickMapping::BUTTON_KICK) && stick.buttons()[m.button(JoystickMapping::BUTTON_KICK)]) {
		kicker_panel.fire();
	}
}

void TesterWindow::on_joystick_scram_changed() {
	const Joystick &stick = *mapped_joysticks->get_device(static_cast<std::size_t>(joystick_chooser.get_active_row_number()));
	const JoystickMapping &m = mapped_joysticks->get_mapping(stick);
	if (m.has_button(JoystickMapping::BUTTON_SCRAM) && stick.buttons()[m.button(JoystickMapping::BUTTON_SCRAM)]) {
		scram();
	}
}

bool TesterWindow::on_delete_event(GdkEventAny *) {
	scram();
	return false;
}

