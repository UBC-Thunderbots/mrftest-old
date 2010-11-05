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
		 * A pointer to a SingleBotComboBoxModel.
		 */
		typedef Glib::RefPtr<SingleBotComboBoxModel> Ptr;

		/**
		 * A column containing the robot's XBee address.
		 */
		Gtk::TreeModelColumn<Glib::ustring> address_column;

		/**
		 * A column containing the numerical index of the robot's lid pattern.
		 */
		Gtk::TreeModelColumn<unsigned int> pattern_index_column;

		/**
		 * Constructs a new SingleRobotComboBoxModel.
		 *
		 * \param[in] robots the robots to display.
		 */
		static Ptr create(const Config::RobotSet &robots);

	private:
		const Config::RobotSet &robots;

		SingleBotComboBoxModel(const Config::RobotSet &robots);
		std::size_t alm_rows() const;
		void alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const;
		void alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &);
		void on_robot_added(std::size_t);
		void on_robot_removed(std::size_t);
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
		 * \param[in] robot the pattern index of the robot to select initially.
		 */
		SingleBotComboBox(const Config::RobotSet &robots, unsigned int robot);

		/**
		 * Returns the address of the currently-selected robot.
		 *
		 * \return the address of the currently-selected robot, or 0 if no robot is currently selected.
		 */
		uint64_t address() const;

	private:
		const Config::RobotSet &robots;
};

#endif

