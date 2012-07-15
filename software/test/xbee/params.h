#ifndef TEST_XBEE_PARAMS_H
#define TEST_XBEE_PARAMS_H

#include "xbee/robot.h"
#include <memory>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/table.h>

/**
 * \brief A panel that allows the user to edit the operational parameters of a robot.
 */
class TesterParamsPanel : public Gtk::Table {
	public:
		/**
		 * \brief Constructs a new TesterParamsPanel.
		 *
		 * \param[in] robot the robot whose parameters should be edited.
		 */
		TesterParamsPanel(XBeeRobot &robot);

	private:
		XBeeRobot &robot;
		XBeeRobot::OperationalParameters::FlashContents flash_contents;
		Gtk::Label channel0label, channel1label;
		Gtk::ComboBoxText channels[2];
		Gtk::Label index_label;
		Gtk::ComboBoxText index;
		Gtk::VBox vbox;
		Gtk::HButtonBox hbb;
		Gtk::Button commit, reboot;
		Gtk::HBox test_mode_hbox;
		Gtk::Label test_mode_label;
		Gtk::Entry test_mode;
		Gtk::Button set_test_mode;
		Gtk::HBox build_signatures_hbox;
		Gtk::Label build_signatures_label, firmware_signature_label, flash_signature_label;
		bool freeze;
		std::unique_ptr<AsyncOperation<XBeeRobot::BuildSignatures>> read_build_signatures_op;
		std::unique_ptr<AsyncOperation<XBeeRobot::OperationalParameters>> read_opparams_op;
		std::unique_ptr<AsyncOperation<void>> write_opparams_op, commit_opparams_op, reboot_op;

		void activate_controls(bool act = true);
		void on_alive_changed();
		void on_read_build_signatures_done(AsyncOperation<XBeeRobot::BuildSignatures> &op);
		void on_read_done(AsyncOperation<XBeeRobot::OperationalParameters> &op);
		void on_change();
		void on_change_done(AsyncOperation<void> &op);
		void on_commit();
		void on_commit_done(AsyncOperation<void> &op);
		void on_reboot();
		void on_reboot_done(AsyncOperation<void> &op);
		void on_test_mode_edited();
		void on_set_test_mode();
};

#endif

