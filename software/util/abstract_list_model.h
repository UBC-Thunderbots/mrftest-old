#ifndef UTIL_ABSTRACT_LIST_MODEL_H
#define UTIL_ABSTRACT_LIST_MODEL_H

#include <gtkmm.h>

//
// An abstract implementation of Gtk::TreeModel that provides a starting point
// for implementing list-based models based on data structures that can be
// accessed randomly by index (such as std::vectors).
//
// Your own class must also extends Glib::Object and invoke the following
// constructors from its own:
//
// Glib::ObjectBase(typeid(your_class_name))
// Glib::Object()
//
// The ObjectBase initialization is permitted because Glib::Object virtually
// extends Glib::ObjectBase, and hence responsibility for invoking
// Glib::ObjectBase's constructor is given to the type of the object being
// constructed, not the immediate subclass of Glib::ObjectBase.
//
// You must override all pure-virtual functions as described below.
//
// You must arrange to invoke alm_row_changed, alm_row_inserted,
// alm_row_deleted, and alm_rows_reordered at the appropriate times.
//
class abstract_list_model : public Gtk::TreeModel {
	public:
		//
		// Constructs a new abstract_list_model. You are expected to call this
		// constructor from your subclass.
		//
		abstract_list_model();

		//
		// Destroys an abstract_list_model.
		//
		virtual ~abstract_list_model();

	protected:
		//
		// You must override this and have it return the number of rows in the
		// data model.
		//
		virtual unsigned int alm_rows() const = 0;

		//
		// You must override this and have it fetch the object at the given
		// location.
		//
		virtual void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const = 0;

		//
		// You must override this and have it store the object at the given
		// location (or do nothing if your model is not editable).
		//
		virtual void alm_set_value(unsigned int row, unsigned int col, const Glib::ValueBase &value) = 0;

		//
		// You must call this when you change the contents of a row.
		//
		void alm_row_changed(unsigned int index);

		//
		// You must call this when you add a row.
		//
		void alm_row_inserted(unsigned int index);

		//
		// You must call this when you remove a row.
		//
		void alm_row_deleted(unsigned int index);

		//
		// You must call this when you reorder rows.
		//
		void alm_rows_reordered(const Glib::ArrayHandle<int> &new_order);

		//
		// You must add all your Gtk::TreeModelColumn objects to this record.
		//
		ColumnRecord alm_column_record;

	private:
		int stamp;

		Gtk::TreeModelFlags get_flags_vfunc() const;
		int get_n_columns_vfunc() const;
		GType get_column_type_vfunc(int col) const;
		bool iter_next_vfunc(const iterator &, iterator &) const;
		bool get_iter_vfunc(const Path &, iterator &) const;
		bool iter_children_vfunc(const iterator &, iterator &) const;
		bool iter_parent_vfunc(const iterator &, iterator &) const;
		bool iter_nth_child_vfunc(const iterator &, int, iterator &) const;
		bool iter_nth_root_child_vfunc(int n, iterator &) const;
		bool iter_has_child_vfunc(const iterator &) const;
		int iter_n_children_vfunc(const iterator &) const;
		int iter_n_root_children_vfunc() const;
		Path get_path_vfunc(const iterator &) const;
		bool iter_is_valid(const iterator &) const;
		void get_value_vfunc(const iterator &, int, Glib::ValueBase &) const;
		void set_value_impl(const iterator &, int, const Glib::ValueBase &);

		void make_iter_valid(iterator &, unsigned int) const;
		static void make_iter_invalid(iterator &);
		bool is_valid_column(int col) const;
};

#endif

