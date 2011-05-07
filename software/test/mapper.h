#ifndef TEST_MAPPER_H
#define TEST_MAPPER_H

#include "test/mapping.h"
#include "util/joystick.h"
#include <gtkmm.h>
#include <vector>

/**
 * \brief A window that allows the user to set up joystick mappings.
 */
class MapperWindow : public Gtk::Window {
	public:
		/**
		 * \brief Constructs a new MapperWindow.
		 */
		MapperWindow();

		/**
		 * \brief Destroys a MapperWindow.
		 */
		~MapperWindow();

	private:
		class MappingsListModel;
		class PreviewDevicesModel;

		Glib::RefPtr<MappingsListModel> mappings;
		Glib::RefPtr<PreviewDevicesModel> preview_devices;

		Gtk::HPaned hpaned;

		Gtk::VBox left_vbox;
		Gtk::ScrolledWindow name_chooser_scroll;
		Gtk::TreeView name_chooser;
		Gtk::HButtonBox hbb;
		Gtk::Button add_button, del_button;

		Gtk::VBox right_vbox;
		Gtk::Frame axes_frame;
		Gtk::Table axes_table;
		Gtk::Label axis_labels[JoystickMapping::N_AXES];
		Gtk::SpinButton axis_spinners[JoystickMapping::N_AXES];
		Gtk::ProgressBar axis_indicators[JoystickMapping::N_AXES];
		Gtk::Frame buttons_frame;
		Gtk::Table buttons_table;
		Gtk::Label button_labels[JoystickMapping::N_BUTTONS];
		Gtk::SpinButton button_spinners[JoystickMapping::N_BUTTONS];
		Gtk::CheckButton button_indicators[JoystickMapping::N_BUTTONS];
		Gtk::ComboBox preview_device_chooser;
		sigc::connection preview_device_connection;

		std::vector<sigc::connection> conns;

		void on_add_clicked();
		void on_del_clicked();
		void on_name_chooser_sel_changed();
		void on_axis_changed(unsigned int axis);
		void on_button_changed(unsigned int button);
		void on_preview_device_changed();
		void update_preview();
		bool on_delete_event(GdkEventAny *);
};

#endif

