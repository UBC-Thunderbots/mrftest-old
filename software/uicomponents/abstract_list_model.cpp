#include "uicomponents/abstract_list_model.h"
#include <cassert>
#include <stdint.h>

abstract_list_model::abstract_list_model() : stamp(1) {
}

abstract_list_model::~abstract_list_model() {
}

void abstract_list_model::alm_row_changed(unsigned int index) {
	Gtk::TreePath path;
	path.push_back(index);
	iterator iter;
	iter.set_stamp(stamp);
	iter.gobj()->user_data = reinterpret_cast<void *>(index);
	row_changed(path, iter);
}

void abstract_list_model::alm_row_inserted(unsigned int index) {
	Gtk::TreePath path;
	path.push_back(index);
	iterator iter;
	iter.set_stamp(stamp);
	iter.gobj()->user_data = reinterpret_cast<void *>(index);
	row_inserted(path, iter);
}

void abstract_list_model::alm_row_deleted(unsigned int index) {
	Gtk::TreePath path;
	path.push_back(index);
	row_deleted(path);
}

Gtk::TreeModelFlags abstract_list_model::get_flags_vfunc() const {
	return Gtk::TREE_MODEL_LIST_ONLY;
}

int abstract_list_model::get_n_columns_vfunc() const {
	return alm_column_record.size();
}

GType abstract_list_model::get_column_type_vfunc(int col) const {
	assert(is_valid_column(col));
	return alm_column_record.types()[col];
}

bool abstract_list_model::iter_next_vfunc(const iterator &iter, iterator &next) const {
	assert(iter_is_valid(iter));
	unsigned int nextindex = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(iter.gobj()->user_data)) + 1U;
	if (nextindex < alm_rows()) {
		make_iter_valid(next, nextindex);
		return true;
	} else {
		make_iter_invalid(next);
		return false;
	}
}

bool abstract_list_model::get_iter_vfunc(const Path &path, iterator &iter) const {
	if (path.size() == 1 && static_cast<unsigned int>(path[0]) < alm_rows()) {
		make_iter_valid(iter, path[0]);
		return true;
	} else {
		make_iter_invalid(iter);
		return false;
	}
}

bool abstract_list_model::iter_children_vfunc(const iterator &, iterator &next) const {
	make_iter_invalid(next);
	return false;
}

bool abstract_list_model::iter_parent_vfunc(const iterator &, iterator &next) const {
	make_iter_invalid(next);
	return false;
}

bool abstract_list_model::iter_nth_child_vfunc(const iterator &, int, iterator &next) const {
	make_iter_invalid(next);
	return false;
}

bool abstract_list_model::iter_nth_root_child_vfunc(int n, iterator &next) const {
	if (static_cast<unsigned int>(n) < alm_rows()) {
		make_iter_valid(next, n);
		return true;
	} else {
		make_iter_invalid(next);
		return false;
	}
}

bool abstract_list_model::iter_has_child_vfunc(const iterator &) const {
	return false;
}

int abstract_list_model::iter_n_children_vfunc(const iterator &) const {
	return 0;
}

int abstract_list_model::iter_n_root_children_vfunc() const {
	return alm_rows();
}

Gtk::TreeModel::Path abstract_list_model::get_path_vfunc(const iterator &iter) const {
	assert(iter_is_valid(iter));
	Path p;
	p.push_back(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(iter.gobj()->user_data)));
	return p;
}

bool abstract_list_model::iter_is_valid(const iterator &iter) const {
	return iter.get_stamp() == stamp && static_cast<unsigned int>(reinterpret_cast<uintptr_t>(iter.gobj()->user_data)) < alm_rows();
}

void abstract_list_model::make_iter_valid(iterator &iter, unsigned int index) const {
	iter.set_stamp(stamp);
	iter.gobj()->user_data = reinterpret_cast<void *>(index);
}

void abstract_list_model::make_iter_invalid(iterator &iter) {
	iter.set_stamp(0);
	iter.gobj()->user_data = 0;
}

void abstract_list_model::get_value_vfunc(const iterator &iter, int col, Glib::ValueBase &value) const {
	assert(iter_is_valid(iter));
	assert(is_valid_column(col));
	unsigned long int index = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(iter.gobj()->user_data));
	alm_get_value(index, col, value);
}

void abstract_list_model::set_value_impl(const iterator &iter, int col, const Glib::ValueBase &value) {
	assert(iter_is_valid(iter));
	assert(is_valid_column(col));
	unsigned long int index = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(iter.gobj()->user_data));
	alm_set_value(index, col, value);
}

bool abstract_list_model::is_valid_column(int col) const {
	return static_cast<unsigned int>(col) < alm_column_record.size();
}

