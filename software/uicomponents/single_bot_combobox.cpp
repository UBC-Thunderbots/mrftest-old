#include "uicomponents/single_bot_combobox.h"
#include "util/dprint.h"
#include <cassert>

SingleBotComboBoxModel::Ptr SingleBotComboBoxModel::create(const Config::RobotSet &robots) {
	Ptr p(new SingleBotComboBoxModel(robots));
	return p;
}

SingleBotComboBoxModel::SingleBotComboBoxModel(const Config::RobotSet &robots) : Glib::ObjectBase(typeid(SingleBotComboBoxModel)), robots(robots) {
	alm_column_record.add(address_column);
	alm_column_record.add(pattern_index_column);
	robots.signal_robot_added.connect(sigc::mem_fun(this, &SingleBotComboBoxModel::on_robot_added));
	robots.signal_robot_removed.connect(sigc::mem_fun(this, &SingleBotComboBoxModel::on_robot_removed));
}

std::size_t SingleBotComboBoxModel::alm_rows() const {
	return robots.size();
}

void SingleBotComboBoxModel::alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const {
	if (col == static_cast<unsigned int>(address_column.index())) {
		Glib::Value<Glib::ustring> v;
		v.init(address_column.type());
		v.set(tohex(robots[row].address, 16));
		value.init(address_column.type());
		value = v;
	} else if (col == static_cast<unsigned int>(pattern_index_column.index())) {
		Glib::Value<unsigned int> v;
		v.init(pattern_index_column.type());
		v.set(robots[row].pattern);
		value.init(pattern_index_column.type());
		value = v;
	}
}

void SingleBotComboBoxModel::alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &) {
}

void SingleBotComboBoxModel::on_robot_added(std::size_t index) {
	alm_row_inserted(index);
}

void SingleBotComboBoxModel::on_robot_removed(std::size_t index) {
	alm_row_deleted(index);
}

SingleBotComboBox::SingleBotComboBox(const Config::RobotSet &robots) : Gtk::ComboBox(SingleBotComboBoxModel::create(robots)), robots(robots) {
	assert(robots.size());
	Glib::RefPtr<SingleBotComboBoxModel> model = Glib::RefPtr<SingleBotComboBoxModel>::cast_static(get_model());
	pack_start(model->address_column, false);
	pack_start(model->pattern_index_column, false);
	set_active(0);
}

SingleBotComboBox::SingleBotComboBox(const Config::RobotSet &robots, unsigned int robot) : Gtk::ComboBox(SingleBotComboBoxModel::create(robots)), robots(robots) {
	assert(robots.size());
	Glib::RefPtr<SingleBotComboBoxModel> model = Glib::RefPtr<SingleBotComboBoxModel>::cast_static(get_model());
	pack_start(model->address_column, false);
	pack_start(model->pattern_index_column, false);
	bool found = false;
	for (unsigned int i = 0; i < robots.size() && !found; ++i) {
		if (robots[i].pattern == robot) {
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

