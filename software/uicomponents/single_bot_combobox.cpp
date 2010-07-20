#include "uicomponents/single_bot_combobox.h"
#include "util/dprint.h"
#include <cassert>

Glib::RefPtr<SingleBotComboBoxModel> SingleBotComboBoxModel::create(const Config::RobotSet &robots) {
	Glib::RefPtr<SingleBotComboBoxModel> p(new SingleBotComboBoxModel(robots));
	return p;
}

SingleBotComboBoxModel::SingleBotComboBoxModel(const Config::RobotSet &robots) : Glib::ObjectBase(typeid(SingleBotComboBoxModel)), robots(robots) {
	alm_column_record.add(address_column);
	alm_column_record.add(yellow_column);
	alm_column_record.add(pattern_index_column);
	alm_column_record.add(name_column);
	robots.signal_robot_added.connect(sigc::mem_fun(this, &SingleBotComboBoxModel::on_robot_added));
	robots.signal_robot_removed.connect(sigc::mem_fun(this, &SingleBotComboBoxModel::on_robot_removed));
}

unsigned int SingleBotComboBoxModel::alm_rows() const {
	return robots.size();
}

void SingleBotComboBoxModel::alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const {
	if (col == 0) {
		Glib::Value<Glib::ustring> v;
		v.init(address_column.type());
		v.set(tohex(robots[row].address, 16));
		value.init(address_column.type());
		value = v;
	} else if (col == 1) {
		Glib::Value<Glib::ustring> v;
		v.init(yellow_column.type());
		v.set(robots[row].yellow ? "Y" : "B");
		value.init(yellow_column.type());
		value = v;
	} else if (col == 2) {
		Glib::Value<unsigned int> v;
		v.init(pattern_index_column.type());
		v.set(robots[row].pattern_index);
		value.init(pattern_index_column.type());
		value = v;
	} else if (col == 3) {
		Glib::Value<Glib::ustring> v;
		v.init(name_column.type());
		v.set(robots[row].name);
		value.init(name_column.type());
		value = v;
	}
}

void SingleBotComboBoxModel::alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &) {
}

void SingleBotComboBoxModel::on_robot_added(unsigned int index) {
	alm_row_inserted(index);
}

void SingleBotComboBoxModel::on_robot_removed(unsigned int index) {
	alm_row_deleted(index);
}

SingleBotComboBox::SingleBotComboBox(const Config::RobotSet &robots) : Gtk::ComboBox(SingleBotComboBoxModel::create(robots)), robots(robots) {
	assert(robots.size());
	Glib::RefPtr<SingleBotComboBoxModel> model = Glib::RefPtr<SingleBotComboBoxModel>::cast_static(get_model());
	pack_start(model->address_column, false);
	pack_start(model->yellow_column, false);
	pack_start(model->pattern_index_column, false);
	pack_start(model->name_column, true);
	set_active(0);
}

SingleBotComboBox::SingleBotComboBox(const Config::RobotSet &robots, const Glib::ustring &robot) : Gtk::ComboBox(SingleBotComboBoxModel::create(robots)), robots(robots) {
	assert(robots.size());
	Glib::RefPtr<SingleBotComboBoxModel> model = Glib::RefPtr<SingleBotComboBoxModel>::cast_static(get_model());
	pack_start(model->address_column, false);
	pack_start(model->yellow_column, false);
	pack_start(model->pattern_index_column, false);
	pack_start(model->name_column, true);
	bool found = false;
	for (unsigned int i = 0; i < robots.size() && !found; ++i) {
		if (robots[i].name == robot) {
			set_active(i);
			found = true;
		}
	}
	if (!found) {
		set_active(0);
	}
}

uint64_t SingleBotComboBox::address() const {
	return robots[get_active_row_number()].address;
}

