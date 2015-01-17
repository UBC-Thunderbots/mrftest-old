#include "uicomponents/abstract_list_model.h"
#include <cassert>
#include <cstdint>
#include <gtkmm/treepath.h>

namespace {
	void *to_ptr(std::size_t sz) {
		return reinterpret_cast<void *>(sz);
	}

	std::size_t of_ptr(void *p) {
		return reinterpret_cast<std::size_t>(p);
	}
}

AbstractListModel::AbstractListModel() : stamp(1) {
}

AbstractListModel::~AbstractListModel() = default;

void AbstractListModel::alm_row_changed(std::size_t index) {
	Gtk::TreePath path;
	path.push_back(static_cast<int>(index));
	iterator iter;
	iter.set_stamp(stamp);
	iter.gobj()->user_data = to_ptr(index);
	row_changed(path, iter);
}

void AbstractListModel::alm_row_inserted(std::size_t index) {
	Gtk::TreePath path;
	path.push_back(static_cast<int>(index));
	iterator iter;
	iter.set_stamp(stamp);
	iter.gobj()->user_data = to_ptr(index);
	row_inserted(path, iter);
}

void AbstractListModel::alm_row_deleted(std::size_t index) {
	Gtk::TreePath path;
	path.push_back(static_cast<int>(index));
	row_deleted(path);
}

Gtk::TreeModelFlags AbstractListModel::get_flags_vfunc() const {
	return Gtk::TREE_MODEL_LIST_ONLY;
}

int AbstractListModel::get_n_columns_vfunc() const {
	return static_cast<int>(alm_column_record.size());
}

GType AbstractListModel::get_column_type_vfunc(int col) const {
	assert(is_valid_column(col));
	return alm_column_record.types()[col];
}

bool AbstractListModel::iter_next_vfunc(const iterator &iter, iterator &next) const {
	std::size_t nextindex = of_ptr(iter.gobj()->user_data) + 1;
	if (nextindex < alm_rows()) {
		make_iter_valid(next, nextindex);
		return true;
	} else {
		make_iter_invalid(next);
		return false;
	}
}

bool AbstractListModel::get_iter_vfunc(const Path &path, iterator &iter) const {
	if (path.size() == 1 && static_cast<unsigned int>(path[0]) < alm_rows()) {
		make_iter_valid(iter, static_cast<std::size_t>(path[0]));
		return true;
	} else {
		make_iter_invalid(iter);
		return false;
	}
}

bool AbstractListModel::iter_children_vfunc(const iterator &, iterator &next) const {
	make_iter_invalid(next);
	return false;
}

bool AbstractListModel::iter_parent_vfunc(const iterator &, iterator &next) const {
	make_iter_invalid(next);
	return false;
}

bool AbstractListModel::iter_nth_child_vfunc(const iterator &, int, iterator &next) const {
	make_iter_invalid(next);
	return false;
}

bool AbstractListModel::iter_nth_root_child_vfunc(int n, iterator &next) const {
	if (static_cast<unsigned int>(n) < alm_rows()) {
		make_iter_valid(next, static_cast<std::size_t>(n));
		return true;
	} else {
		make_iter_invalid(next);
		return false;
	}
}

bool AbstractListModel::iter_has_child_vfunc(const iterator &) const {
	return false;
}

int AbstractListModel::iter_n_children_vfunc(const iterator &) const {
	return 0;
}

int AbstractListModel::iter_n_root_children_vfunc() const {
	return static_cast<int>(alm_rows());
}

Gtk::TreeModel::Path AbstractListModel::get_path_vfunc(const iterator &iter) const {
	Path p;
	std::size_t index = of_ptr(iter.gobj()->user_data);
	p.push_back(static_cast<int>(index));
	return p;
}

void AbstractListModel::make_iter_valid(iterator &iter, std::size_t index) const {
	iter.set_stamp(stamp);
	iter.gobj()->user_data = to_ptr(index);
}

void AbstractListModel::make_iter_invalid(iterator &iter) {
	iter.set_stamp(0);
	iter.gobj()->user_data = nullptr;
}

void AbstractListModel::get_value_vfunc(const iterator &iter, int col, Glib::ValueBase &value) const {
	assert(is_valid_column(col));
	std::size_t index = of_ptr(iter.gobj()->user_data);
	alm_get_value(index, static_cast<unsigned int>(col), value);
}

void AbstractListModel::set_value_impl(const iterator &iter, int col, const Glib::ValueBase &value) {
	assert(is_valid_column(col));
	std::size_t index = of_ptr(iter.gobj()->user_data);
	alm_set_value(index, static_cast<unsigned int>(col), value);
}

bool AbstractListModel::is_valid_column(int col) const {
	return static_cast<unsigned int>(col) < alm_column_record.size();
}

