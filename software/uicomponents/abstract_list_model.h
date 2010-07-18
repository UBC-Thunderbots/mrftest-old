#ifndef UICOMPONENTS_ABSTRACT_LIST_MODEL_H
#define UICOMPONENTS_ABSTRACT_LIST_MODEL_H

#include <gtkmm.h>

/**
 * An abstract implementation of \c Gtk::TreeModel that provides a starting
 * %point for implementing list-based models based on data structures that can
 * be accessed randomly by index (such as \c std::vector).
 *
 * Your own class must also extend \c Glib::Object and invoke the following
 * constructors from its own:
 *
 * \li <code>Glib::ObjectBase(typeid(your_class_name))</code>
 * \li <code>Glib::Object()</code>
 *
 * The \c ObjectBase initialization is permitted because \c Glib::Object
 * virtually extends \c Glib::ObjectBase, and hence responsibility for invoking
 * the \c Glib::ObjectBase constructor is given to the type of the object being
 * constructed, not the immediate subclass of \c Glib::ObjectBase.
 *
 * You must override all pure-virtual functions as described below.
 *
 * You must arrange to invoke alm_row_changed(), alm_row_inserted(),
 * alm_row_deleted(), and alm_rows_reordered() at the appropriate times.
 */
class AbstractListModel : public Gtk::TreeModel {
	protected:
		/**
		 * Constructs a new AbstractListModel. You are expected to call this
		 * constructor from your subclass.
		 */
		AbstractListModel();

		/**
		 * Destroys an AbstractListModel.
		 */
		virtual ~AbstractListModel();

		/**
		 * You must override this method.
		 *
		 * \return the number of rows in the data model.
		 */
		virtual unsigned int alm_rows() const = 0;

		/**
		 * You must override this.
		 *
		 * \param[in] row the row number of the cell whose data should be
		 * fetched.
		 *
		 * \param[in] col the column number of the cell whose data should be
		 * fetched.
		 *
		 * \param[out] value the location where the value from the data model
		 * should be stored.
		 */
		virtual void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const = 0;

		/**
		 * You must override this. If your data model should be read-only, you
		 * should do nothing in this function.
		 *
		 * \param[in] row the row number of the cell whose data should be
		 * stored.
		 *
		 * \param[in] col the column number of the cell whose data should be
		 * stored.
		 *
		 * \param[in] value the value to store.
		 */
		virtual void alm_set_value(unsigned int row, unsigned int col, const Glib::ValueBase &value) = 0;

		/**
		 * Notifies listeners that a row has been modified. You must call this
		 * function when a row is modified.
		 *
		 * \param[in] index the row number that was modified.
		 */
		void alm_row_changed(unsigned int index);

		/**
		 * Notifies listeners that a new row has been added. You must call this
		 * function when a row is added.
		 *
		 * \param[in] index the row number that was added.
		 */
		void alm_row_inserted(unsigned int index);

		/**
		 * Notifies listeners that a row has been deleted. You must call this
		 * function when a row is deleted.
		 *
		 * \param[in] index the row number the row had before it was deleted.
		 */
		void alm_row_deleted(unsigned int index);

		/**
		 * A column record for storing the columns of the model. You must create
		 * \c Gtk::TreeModelColumn objects and add them to this record.
		 */
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

