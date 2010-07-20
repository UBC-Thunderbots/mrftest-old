#ifndef SINGLE_BOT_COMBOBOX_H
#define SINGLE_BOT_COMBOBOX_H

#include "uicomponents/abstract_list_model.h"
#include "util/config.h"
#include <gtkmm.h>

/**
 * The tree model used by a SingleBotComboBox.
 */
class SingleBotComboBoxModel : public Glib::Object, public AbstractListModel {
	public:
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
		static Glib::RefPtr<SingleBotComboBoxModel> create(const Config::RobotSet &robots);

	private:
		const Config::RobotSet &robots;

		SingleBotComboBoxModel(const Config::RobotSet &robots);
		unsigned int alm_rows() const;
		void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const;
		void alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &);
		void on_robot_added(unsigned int);
		void on_robot_removed(unsigned int);
};

/**
 * A combo box that allows the user to select a single robot.
 */
class SingleBotComboBox : public Gtk::ComboBox {
	public:
		/**
		 * Constructs a new SingleBotComboBox.
		 *
		 * \param[in] robots the robots to display in the box.
		 */
		SingleBotComboBox(const Config::RobotSet &robots);

		/**
		 * Constructs a new SingleBotComboBox.
		 *
		 * \param[in] robots the robots to display in the box.
		 *
		 * \param[in] robot the name of the robot to select initially.
		 */
		SingleBotComboBox(const Config::RobotSet &robots, const Glib::ustring &robot);

		/**
		 * \return the address of the currently-selected robot, or 0 if no robot
		 * is currently selected.
		 */
		uint64_t address() const;

	private:
		const Config::RobotSet &robots;
};

#endif

