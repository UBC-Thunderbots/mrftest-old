#ifndef UICOMPONENTS_ABSTRACT_LIST_MODEL_H
#define UICOMPONENTS_ABSTRACT_LIST_MODEL_H

#include <glibmm/value.h>
#include <gtkmm/treemodel.h>
#include <cstddef>

/**
 * An abstract implementation of \c Gtk::TreeModel for list-type models. This
 * class can be efficiently used with random-access data structures such as \c
 * std::vector.
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
 * Note that your class must inherit from \c Glib::Object \em after it inherits
 * from \c AbstractListModel.
 *
 * You must override all pure-virtual functions as described below.
 *
 * You must arrange to invoke alm_row_changed(), alm_row_inserted(), and
 * alm_row_deleted() at the appropriate times.
 */
class AbstractListModel : public Gtk::TreeModel
{
   protected:
    /**
     * Constructs a new AbstractListModel.
     * You are expected to call this constructor from your subclass.
     */
    explicit AbstractListModel();

    /**
     * Destroys an AbstractListModel.
     */
    virtual ~AbstractListModel();

    /**
     * Gets the number of rows in the model.
     * You must override this.
     *
     * \return the number of rows in the data model.
     */
    virtual std::size_t alm_rows() const = 0;

    /**
     * Gets the value at a position in the model.
     * You must override this.
     *
     * \param[in] row the row number of the cell whose data should be fetched.
     *
     * \param[in] col the column number of the cell whose data should be
     * fetched.
     *
     * \param[out] value the location where the value from the data model should
     * be stored.
     */
    virtual void alm_get_value(
        std::size_t row, unsigned int col, Glib::ValueBase &value) const = 0;

    /**
     * Sets the value at a position in the model.
     * You must override this.
     * If your data model should be read-only, you should do nothing in this
     * function.
     *
     * \param[in] row the row number of the cell whose data should be stored.
     *
     * \param[in] col the column number of the cell whose data should be stored.
     *
     * \param[in] value the value to store.
     */
    virtual void alm_set_value(
        std::size_t row, unsigned int col, const Glib::ValueBase &value) = 0;

    /**
     * Notifies listeners that a row has been modified.
     * You must call this function when a row is modified.
     *
     * \param[in] index the row number that was modified.
     */
    void alm_row_changed(std::size_t index);

    /**
     * Notifies listeners that a new row has been added.
     * You must call this function when a row is added.
     *
     * \param[in] index the row number that was added.
     */
    void alm_row_inserted(std::size_t index);

    /**
     * Notifies listeners that a row has been deleted.
     * You must call this function when a row is deleted.
     *
     * \param[in] index the row number the row had before it was deleted.
     */
    void alm_row_deleted(std::size_t index);

    /**
     * A column record for storing the columns of the model.
     * You must create \c Gtk::TreeModelColumn objects and add them to this
     * record.
     */
    ColumnRecord alm_column_record;

   private:
    int stamp;

    Gtk::TreeModelFlags get_flags_vfunc() const override;
    int get_n_columns_vfunc() const override;
    GType get_column_type_vfunc(int col) const override;
    bool iter_next_vfunc(const iterator &, iterator &) const override;
    bool get_iter_vfunc(const Path &, iterator &) const override;
    bool iter_children_vfunc(const iterator &, iterator &) const override;
    bool iter_parent_vfunc(const iterator &, iterator &) const override;
    bool iter_nth_child_vfunc(const iterator &, int, iterator &) const override;
    bool iter_nth_root_child_vfunc(int n, iterator &) const override;
    bool iter_has_child_vfunc(const iterator &) const override;
    int iter_n_children_vfunc(const iterator &) const override;
    int iter_n_root_children_vfunc() const override;
    Path get_path_vfunc(const iterator &) const override;
    void get_value_vfunc(
        const iterator &, int, Glib::ValueBase &) const override;
    void set_value_impl(
        const iterator &, int, const Glib::ValueBase &) override;

    void make_iter_valid(iterator &, std::size_t) const;
    static void make_iter_invalid(iterator &);
    bool is_valid_column(int col) const;
};

#endif
