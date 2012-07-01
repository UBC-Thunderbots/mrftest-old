#include "test/mapper.h"
#include "uicomponents/abstract_list_model.h"
#include "util/config.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <unordered_set>
#include <vector>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <gtkmm/comboboxentrytext.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>

using namespace std::placeholders;

namespace {
	bool format_spin_output(Gtk::SpinButton &btn) {
		int i = btn.get_value_as_int();
		if (i >= 0) {
			btn.set_text(Glib::ustring::format(i));
		} else {
			btn.set_text("<None>");
		}
		return true;
	}

	bool has_selected_row(const Gtk::TreeView &tv) {
		return tv.get_selection()->count_selected_rows() > 0;
	}

	std::size_t get_selected_row(const Gtk::TreeView &tv) {
		const Gtk::TreeSelection::ListHandle_Path &sel = tv.get_selection()->get_selected_rows();
		assert(!sel.empty());
		const Gtk::TreePath &p = *sel.begin();
		return static_cast<std::size_t>(p[0]);
	}
}

class MapperWindow::MappingsListModel : public Glib::Object, public AbstractListModel {
	public:
		std::vector<JoystickMapping> mappings;

		Gtk::TreeModelColumn<Glib::ustring> name_column;

		static Glib::RefPtr<MappingsListModel> create() {
			Glib::RefPtr<MappingsListModel> p(new MappingsListModel);
			return p;
		}

		void save() {
			xmlpp::Element *joysticks_elt = Config::joysticks();
			{
				const xmlpp::Node::NodeList &to_remove = joysticks_elt->get_children();
				std::for_each(to_remove.begin(), to_remove.end(), [joysticks_elt](xmlpp::Node *n) { joysticks_elt->remove_child(n); });
			}
			for (auto i = mappings.begin(), iend = mappings.end(); i != iend; ++i) {
				i->save(joysticks_elt->add_child("joystick"));
			}
			Config::save();
		}

		std::size_t alm_rows() const {
			return mappings.size();
		}

		void alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const {
			if (col == static_cast<unsigned int>(name_column.index())) {
				Glib::Value<Glib::ustring> v;
				v.init(name_column.type());
				v.set(mappings[row].name());
				value.init(name_column.type());
				value = v;
			} else {
				std::abort();
			}
		}

		void alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &) {
		}

		bool has_mapping(const Glib::ustring &name) const {
			return std::binary_search(mappings.begin(), mappings.end(), JoystickMapping(name));
		}

		std::size_t add_mapping(const Glib::ustring &name) {
			JoystickMapping m(name);
			auto iter = std::lower_bound(mappings.begin(), mappings.end(), m);
			assert(!(iter != mappings.end() && iter->name() == name));
			std::size_t index = static_cast<std::size_t>(iter - mappings.begin());
			mappings.insert(iter, m);
			alm_row_inserted(index);
			return index;
		}

		void remove_mapping(std::size_t index) {
			assert(index < mappings.size());
			mappings.erase(mappings.begin() + static_cast<std::vector<JoystickMapping>::difference_type>(index));
			alm_row_deleted(index);
		}

	private:
		MappingsListModel() : Glib::ObjectBase(typeid(MappingsListModel)) {
			alm_column_record.add(name_column);

			const xmlpp::Element *joysticks_elt = Config::joysticks();
			const xmlpp::Node::NodeList &joystick_elts = joysticks_elt->get_children();
			for (auto i = joystick_elts.begin(), iend = joystick_elts.end(); i != iend; ++i) {
				const xmlpp::Node *n = *i;
				const xmlpp::Element *e = dynamic_cast<const xmlpp::Element *>(n);
				if (e) {
					if (e->get_name() != "joystick") {
						throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose("Malformed config.xml (expected element of type joystick, found %1)", e->get_name())));
					}
					mappings.push_back(JoystickMapping(e));
				}
			}
			std::sort(mappings.begin(), mappings.end());
			for (auto i = mappings.begin(), iend = mappings.end(); i != iend; ++i) {
				if (i + 1 != iend && i->name() == (i + 1)->name()) {
					throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose("Malformed config.xml (duplicate joystick type %1)", i->name())));
				}
			}
		}
};

class MapperWindow::PreviewDevicesModel : public Glib::Object, public AbstractListModel {
	public:
		Gtk::TreeModelColumn<Glib::ustring> name_column;

		static Glib::RefPtr<PreviewDevicesModel> create() {
			Glib::RefPtr<PreviewDevicesModel> p(new PreviewDevicesModel);
			return p;
		}

		void present(const Glib::ustring &model = Glib::ustring()) {
			while (!devices.empty()) {
				std::size_t sz = devices.size();
				devices.erase(devices.begin() + static_cast<std::vector<const Joystick *>::difference_type>(sz - 1));
				alm_row_deleted(sz);
			}
			if (!model.empty()) {
				for (std::size_t i = 0; i < Joystick::count(); ++i) {
					if (Joystick::get(i).name() == model) {
						devices.push_back(&Joystick::get(i));
						alm_row_inserted(devices.size());
					}
				}
			}
		}

		const Joystick *get_device(std::size_t index) const {
			return index > 0 ? devices[index - 1] : 0;
		}

		std::size_t alm_rows() const {
			return devices.size() + 1;
		}

		void alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const {
			if (col == static_cast<unsigned int>(name_column.index())) {
				Glib::Value<Glib::ustring> v;
				v.init(name_column.type());
				if (row > 0) {
					v.set(Glib::filename_to_utf8(devices[row - 1]->node()));
				} else {
					v.set("<No Preview Device>");
				}
				value.init(name_column.type());
				value = v;
			} else {
				std::abort();
			}
		}

		void alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &) {
		}

	private:
		std::vector<const Joystick *> devices;

		PreviewDevicesModel() : Glib::ObjectBase(typeid(PreviewDevicesModel)) {
			alm_column_record.add(name_column);
		}
};

MapperWindow::MapperWindow() : mappings(MappingsListModel::create()), preview_devices(PreviewDevicesModel::create()), name_chooser(Glib::RefPtr<Gtk::TreeModel>::cast_static(mappings)), add_button(Gtk::Stock::ADD), del_button(Gtk::Stock::REMOVE), axes_frame("Axes"), axes_table(JoystickMapping::N_AXES, 3), buttons_frame("Buttons"), buttons_table(JoystickMapping::N_BUTTONS, 3), preview_device_chooser(Glib::RefPtr<Gtk::TreeModel>::cast_static(preview_devices)) {
	set_title("Joystick Mapper");

	name_chooser.append_column("Model", mappings->name_column);
	name_chooser.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
	conns.push_back(name_chooser.get_selection()->signal_changed().connect(sigc::mem_fun(this, &MapperWindow::on_name_chooser_sel_changed)));
	name_chooser_scroll.add(name_chooser);
	left_vbox.pack_start(name_chooser_scroll, Gtk::PACK_EXPAND_WIDGET);
	add_button.signal_clicked().connect(sigc::mem_fun(this, &MapperWindow::on_add_clicked));
	hbb.pack_start(add_button);
	del_button.set_sensitive(false);
	del_button.signal_clicked().connect(sigc::mem_fun(this, &MapperWindow::on_del_clicked));
	hbb.pack_start(del_button);
	left_vbox.pack_start(hbb, Gtk::PACK_SHRINK);
	hpaned.pack1(left_vbox, true, true);

	for (unsigned int i = 0; i < JoystickMapping::N_AXES; ++i) {
		axis_labels[i].set_text(Glib::ustring::compose("Drive %1:", i + 1));
		axes_table.attach(axis_labels[i], 0, 1, i, i + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
		axis_spinners[i].set_sensitive(false);
		axis_spinners[i].signal_output().connect(sigc::bind(&format_spin_output, sigc::ref(axis_spinners[i])));
		axis_spinners[i].get_adjustment()->configure(-1, -1, 255, 1, 10, 0);
		conns.push_back(axis_spinners[i].get_adjustment()->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this, &MapperWindow::on_axis_changed), i)));
		axis_spinners[i].set_update_policy(Gtk::UPDATE_IF_VALID);
		axis_spinners[i].set_width_chars(10);
		axes_table.attach(axis_spinners[i], 1, 2, i, i + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
		axes_table.attach(axis_indicators[i], 2, 3, i, i + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::SHRINK);
	}
	axes_frame.add(axes_table);
	right_vbox.pack_start(axes_frame, Gtk::PACK_SHRINK);
	for (unsigned int i = 0; i < JoystickMapping::N_BUTTONS; ++i) {
		button_labels[i].set_text(Glib::ustring::compose("%1:", JoystickMapping::BUTTON_LABELS[i]));
		buttons_table.attach(button_labels[i], 0, 1, i, i + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
		button_spinners[i].set_sensitive(false);
		button_spinners[i].signal_output().connect(sigc::bind(&format_spin_output, sigc::ref(button_spinners[i])));
		button_spinners[i].get_adjustment()->configure(-1, -1, 255, 1, 10, 0);
		conns.push_back(button_spinners[i].get_adjustment()->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this, &MapperWindow::on_button_changed), i)));
		button_spinners[i].set_update_policy(Gtk::UPDATE_IF_VALID);
		button_spinners[i].set_width_chars(10);
		buttons_table.attach(button_spinners[i], 1, 2, i, i + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::SHRINK);
		button_indicators[i].set_sensitive(false);
		buttons_table.attach(button_indicators[i], 2, 3, i, i + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
	}
	buttons_frame.add(buttons_table);
	right_vbox.pack_start(buttons_frame, Gtk::PACK_SHRINK);
	preview_device_chooser.set_active(0);
	preview_device_chooser.set_sensitive(false);
	preview_device_chooser.pack_start(preview_devices->name_column);
	conns.push_back(preview_device_chooser.signal_changed().connect(sigc::mem_fun(this, &MapperWindow::on_preview_device_changed)));
	right_vbox.pack_start(preview_device_chooser, Gtk::PACK_SHRINK);
	hpaned.pack2(right_vbox, true, true);

	add(hpaned);

	show_all();
}

MapperWindow::~MapperWindow() {
	preview_device_connection.disconnect();
	std::for_each(conns.begin(), conns.end(), [](sigc::connection &conn) { conn.disconnect(); });
}

void MapperWindow::on_add_clicked() {
	Gtk::MessageDialog md(*this, "Which joystick do you want to map?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
	Gtk::ComboBoxEntryText cbet;
	std::unordered_set<std::string> names_used;
	for (std::size_t i = 0; i < Joystick::count(); ++i) {
		const Joystick &js = Joystick::get(i);
		const std::string &ck = js.name().collate_key();
		if (!mappings->has_mapping(js.name()) && !names_used.count(ck)) {
			cbet.append_text(js.name());
			names_used.insert(ck);
		}
	}
	md.get_vbox()->pack_start(cbet, Gtk::PACK_SHRINK);
	cbet.show();
	int rc = md.run();
	if (rc == Gtk::RESPONSE_OK) {
		const Glib::ustring &name = cbet.get_entry()->get_text();
		if (!name.empty()) {
			if (mappings->has_mapping(name)) {
				Gtk::MessageDialog md2(*this, "A mapping already exists for that joystick.", false, Gtk::MESSAGE_ERROR);
				md2.run();
			} else {
				std::size_t pos = mappings->add_mapping(name);
				Gtk::TreePath p;
				p.push_back(static_cast<int>(pos));
				name_chooser.get_selection()->select(p);
			}
		}
	}
}

void MapperWindow::on_del_clicked() {
	if (has_selected_row(name_chooser)) {
		std::size_t row = get_selected_row(name_chooser);
		Gtk::MessageDialog md(*this, Glib::ustring::compose("Delete the joystick %1?", mappings->mappings[row].name()), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
		int rc = md.run();
		if (rc == Gtk::RESPONSE_YES) {
			mappings->remove_mapping(row);
		}
	}
}

void MapperWindow::on_name_chooser_sel_changed() {
	if (has_selected_row(name_chooser)) {
		del_button.set_sensitive();
		const JoystickMapping &m = mappings->mappings[get_selected_row(name_chooser)];
		for (unsigned int i = 0; i < JoystickMapping::N_AXES; ++i) {
			axis_spinners[i].set_sensitive();
			if (m.has_axis(i)) {
				axis_spinners[i].set_value(m.axis(i));
			} else {
				axis_spinners[i].set_value(-1);
			}
		}
		for (unsigned int i = 0; i < JoystickMapping::N_BUTTONS; ++i) {
			button_spinners[i].set_sensitive();
			if (m.has_button(i)) {
				button_spinners[i].set_value(m.button(i));
			} else {
				button_spinners[i].set_value(-1);
			}
		}
		preview_devices->present(m.name());
		preview_device_chooser.set_sensitive();
	} else {
		del_button.set_sensitive(false);
		for (unsigned int i = 0; i < JoystickMapping::N_AXES; ++i) {
			axis_spinners[i].set_sensitive(false);
			axis_spinners[i].set_value(-1);
		}
		for (unsigned int i = 0; i < JoystickMapping::N_BUTTONS; ++i) {
			button_spinners[i].set_sensitive(false);
			button_spinners[i].set_value(-1);
		}
		preview_devices->present();
		preview_device_chooser.set_sensitive(false);
	}
}

void MapperWindow::on_axis_changed(unsigned int i) {
	if (has_selected_row(name_chooser)) {
		JoystickMapping &m = mappings->mappings[get_selected_row(name_chooser)];
		if (axis_spinners[i].get_text() != "<None>") {
			m.set_axis(i, static_cast<unsigned int>(axis_spinners[i].get_value_as_int()));
		} else {
			m.clear_axis(i);
		}
		update_preview();
	}
}

void MapperWindow::on_button_changed(unsigned int i) {
	if (has_selected_row(name_chooser)) {
		JoystickMapping &m = mappings->mappings[get_selected_row(name_chooser)];
		if (button_spinners[i].get_text() != "<None>") {
			m.set_button(i, static_cast<unsigned int>(button_spinners[i].get_value_as_int()));
		} else {
			m.clear_button(i);
		}
		update_preview();
	}
}

void MapperWindow::on_preview_device_changed() {
	preview_device_connection.disconnect();
	const Joystick *device = preview_devices->get_device(static_cast<std::size_t>(preview_device_chooser.get_active_row_number()));
	if (device) {
		preview_device_connection = device->signal_changed().connect(sigc::mem_fun(this, &MapperWindow::update_preview));
	}
}

void MapperWindow::update_preview() {
	const Joystick *device = preview_devices->get_device(static_cast<std::size_t>(preview_device_chooser.get_active_row_number()));
	if (device && has_selected_row(name_chooser)) {
		const JoystickMapping &m = mappings->mappings[get_selected_row(name_chooser)];
		for (unsigned int i = 0; i < JoystickMapping::N_AXES; ++i) {
			if (m.has_axis(i) && m.axis(i) < device->axes().size()) {
				double phys = device->axes()[m.axis(i)];
				axis_indicators[i].set_fraction((phys + 1) / 2);
			} else {
				axis_indicators[i].set_fraction(0);
			}
		}
		for (unsigned int i = 0; i < JoystickMapping::N_BUTTONS; ++i) {
			button_indicators[i].set_active(m.has_button(i) && m.button(i) < device->buttons().size() && device->buttons()[m.button(i)]);
		}
	} else {
		for (unsigned int i = 0; i < JoystickMapping::N_AXES; ++i) {
			axis_indicators[i].set_fraction(0);
		}
		for (unsigned int i = 0; i < JoystickMapping::N_BUTTONS; ++i) {
			button_indicators[i].set_active(false);
		}
	}
}

bool MapperWindow::on_delete_event(GdkEventAny *) {
	mappings->save();
	return false;
}

