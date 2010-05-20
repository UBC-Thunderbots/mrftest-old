#include "uicomponents/single_bot_combobox.h"
#include "util/abstract_list_model.h"
#include "util/dprint.h"
#include <cassert>

single_bot_combobox_model::ptr single_bot_combobox_model::create(const config::robot_set &robots) {
	ptr p(new single_bot_combobox_model(robots));
	return p;
}

single_bot_combobox_model::single_bot_combobox_model(const config::robot_set &robots) : Glib::ObjectBase(typeid(single_bot_combobox_model)), robots(robots) {
	alm_column_record.add(address_column);
	alm_column_record.add(yellow_column);
	alm_column_record.add(pattern_index_column);
	alm_column_record.add(name_column);
	robots.signal_robot_added.connect(sigc::mem_fun(this, &single_bot_combobox_model::on_robot_added));
	robots.signal_robot_removed.connect(sigc::mem_fun(this, &single_bot_combobox_model::on_robot_removed));
}

unsigned int single_bot_combobox_model::alm_rows() const {
	return robots.size();
}

void single_bot_combobox_model::alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const {
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

void single_bot_combobox_model::alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &) {
}

void single_bot_combobox_model::on_robot_added(unsigned int index) {
	alm_row_inserted(index);
}

void single_bot_combobox_model::on_robot_removed(unsigned int index) {
	alm_row_deleted(index);
}

single_bot_combobox::single_bot_combobox(const config::robot_set &robots) : Gtk::ComboBox(single_bot_combobox_model::create(robots)), robots(robots) {
	assert(robots.size());
	Glib::RefPtr<single_bot_combobox_model> model = Glib::RefPtr<single_bot_combobox_model>::cast_static(get_model());
	pack_start(model->address_column, false);
	pack_start(model->yellow_column, false);
	pack_start(model->pattern_index_column, false);
	pack_start(model->name_column, true);
	set_active(0);
}

uint64_t single_bot_combobox::address() const {
	return robots[get_active_row_number()].address;
}

