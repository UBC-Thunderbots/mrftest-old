#ifndef SINGLE_BOT_COMBOBOX_H
#define SINGLE_BOT_COMBOBOX_H

#include "util/abstract_list_model.h"
#include "util/config.h"
#include <gtkmm.h>

/**
 * The tree model used by a single_bot_combobox.
 */
class single_bot_combobox_model : public Glib::Object, public abstract_list_model {
	public:
		/**
		 * A pointer to a single_bot_combobox_model.
		 */
		typedef Glib::RefPtr<single_bot_combobox_model> ptr;

		/**
		 * A column containing the robot's XBee address.
		 */
		Gtk::TreeModelColumn<Glib::ustring> address_column;

		/**
		 * A column containing 'Y' if the robot is yellow, or 'B' if it is blue.
		 */
		Gtk::TreeModelColumn<Glib::ustring> yellow_column;

		/**
		 * A column containing the numerical index of the robot's lid pattern.
		 */
		Gtk::TreeModelColumn<unsigned int> pattern_index_column;

		/**
		 * A column containing the robot's name.
		 */
		Gtk::TreeModelColumn<Glib::ustring> name_column;

		/**
		 * Constructs a new single_robot_combobox_model.
		 * \param robots the robots to display
		 */
		static ptr create(const config::robot_set &robots);

	private:
		const config::robot_set &robots;

		single_bot_combobox_model(const config::robot_set &robots);
		unsigned int alm_rows() const;
		void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const;
		void alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &);
		void on_robot_added(unsigned int);
		void on_robot_removed(unsigned int);
};

/**
 * A combo box that allows the user to select a single robot.
 */
class single_bot_combobox : public Gtk::ComboBox {
	public:
		/**
		 * Constructs a new single_bot_combobox.
		 * \param robots the robots to display in the box
		 */
		single_bot_combobox(const config::robot_set &robots);

		/**
		 * \return the address of the currently-selected robot, or 0 if no robot
		 * is currently selected.
		 */
		uint64_t address() const;

	private:
		const config::robot_set &robots;
};

#endif

