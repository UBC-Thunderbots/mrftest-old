#include "simulator/main_window.h"
#include "uicomponents/single_bot_combobox.h"
#include "util/dprint.h"

namespace {
	/**
	 * Controls for manipulating a single robot in the simulator.
	 */
	class RobotControls : public Gtk::Table {
		public:
			/**
			 * Constructs a new control widget.
			 * \param sim the Simulator to control
			 */
			RobotControls(Simulator &sim) : Gtk::Table(5, 2), sim(sim) {
				unsigned int y = 0;

				power_button.set_sensitive(false);
				power_button.set_label("On");
				power_button.signal_toggled().connect(sigc::mem_fun(this, &RobotControls::on_power_switched));
				add_form_row(power_button, "Robot power:", y);

				field_button.set_sensitive(false);
				field_button.set_label("Place");
				field_button.signal_toggled().connect(sigc::mem_fun(this, &RobotControls::on_field_switched));
				add_form_row(field_button, "On field:", y);

				battery_scale.set_sensitive(false);
				battery_scale.get_adjustment()->configure(0.0, 0.0, 17.0, 0.1, 1.0, 0.0);
				battery_scale.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &RobotControls::on_battery_moved));
				add_form_row(battery_scale, "Battery:", y);

				address64_entry.set_editable(false);
				add_form_row(address64_entry, "64-bit address:", y);

				address16_entry.set_editable(false);
				add_form_row(address16_entry, "16-bit address:", y);

				run_data_offset_entry.set_editable(false);
				add_form_row(run_data_offset_entry, "Run data offset:", y);

				bootload_check.set_sensitive(false);
				bootload_check.set_label("Active");
				add_form_row(bootload_check, "Bootload mode:", y);
			}

			/**
			 * Sets the controls to display information about a robot.
			 * \param address the address of the robot to display
			 */
			void set_robot(uint64_t address) {
				bot = sim.robots().find(address)->second;
				connection.disconnect();
				connection = bot->signal_changed.connect(sigc::mem_fun(this, &RobotControls::on_changed));
				on_changed();
			}

			/**
			 * Sets the controls to disabled and not to show any information.
			 */
			void clear_robot() {
				bot.reset();
				connection.disconnect();
				on_changed();
			}

		private:
			Simulator &sim;
			RefPtr<SimulatorRobot> bot;
			sigc::connection connection;
			Gtk::ToggleButton power_button;
			Gtk::ToggleButton field_button;
			Gtk::HScale battery_scale;
			Gtk::Entry address64_entry;
			Gtk::Entry address16_entry;
			Gtk::Entry run_data_offset_entry;
			Gtk::CheckButton bootload_check;

			void add_form_row(Gtk::Widget &widget, const Glib::ustring &name, unsigned int &y) {
				attach(*Gtk::manage(new Gtk::Label(name)), 0, 1, y, y + 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				attach(widget, 1, 2, y, y + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				++y;
			}

			void on_changed() {
				if (bot) {
					power_button.set_active(bot->powered());
					power_button.set_sensitive(true);
					field_button.set_active(!!bot->get_player());
					field_button.set_sensitive(true);
					battery_scale.set_value(bot->battery());
					battery_scale.set_sensitive(true);
					address64_entry.set_text(tohex(bot->address, 16));
					address16_entry.set_text(tohex(bot->address16(), 4));
					run_data_offset_entry.set_text(Glib::ustring::format(bot->run_data_offset()));
					bootload_check.set_active(bot->bootloading());
				} else {
					power_button.set_active(false);
					power_button.set_sensitive(false);
					field_button.set_active(false);
					field_button.set_sensitive(false);
					battery_scale.set_value(0.0);
					battery_scale.set_sensitive(false);
					address64_entry.set_text("");
					address16_entry.set_text("");
					run_data_offset_entry.set_text("");
					bootload_check.set_active(false);
				}
			}

			void on_power_switched() {
				if (bot) {
					bot->powered(power_button.get_active());
				}
			}

			void on_field_switched() {
				if (bot) {
					if (field_button.get_active()) {
						bot->add_player();
					} else {
						bot->remove_player();
					}
				}
			}

			void on_battery_moved() {
				if (bot) {
					bot->battery(battery_scale.get_value());
				}
			}
	};

	/**
	 * Controls for manipulating the robots in the simulator.
	 */
	class RobotsControls : public Gtk::VBox {
		public:
			/**
			 * Constructs a new robots control widget.
			 * \param sim the Simulator to control
			 */
			RobotsControls(Simulator &sim) : sim(sim), robots_list_model(SingleBotComboBoxModel::create(sim.conf.robots())), robots_list(robots_list_model), controls(sim) {
				robots_list.append_column("Address", robots_list_model->address_column);
				robots_list.append_column("Colour", robots_list_model->yellow_column);
				robots_list.append_column("Pattern", robots_list_model->pattern_index_column);
				robots_list.append_column("Name", robots_list_model->name_column);
				robots_list.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
				robots_list.get_selection()->signal_changed().connect(sigc::mem_fun(this, &RobotsControls::on_list_selection_changed));
				Gtk::ScrolledWindow *scroller = Gtk::manage(new Gtk::ScrolledWindow);
				scroller->set_shadow_type(Gtk::SHADOW_IN);
				scroller->add(robots_list);
				pack_start(*scroller, Gtk::PACK_EXPAND_WIDGET);
				pack_start(controls, Gtk::PACK_SHRINK);
				on_list_selection_changed();
			}

		private:
			Simulator &sim;
			RefPtr<SingleBotComboBoxModel> robots_list_model;
			Gtk::TreeView robots_list;
			RobotControls controls;

			void on_list_selection_changed() {
				const Gtk::TreeSelection::ListHandle_Path &sel = robots_list.get_selection()->get_selected_rows();
				if (sel.size() == 1 && (*sel.begin()).size() == 1) {
					unsigned int index = (*sel.begin())[0];
					uint64_t address = sim.conf.robots()[index].address;
					controls.set_robot(address);
				} else {
					controls.clear_robot();
				}
			}
	};
}

MainWindow::MainWindow(Simulator &sim) : sim(sim), vis(sim.visualizer_data()) {
	set_title("Thunderbots Simulator");

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);

	Gtk::Frame *robots_frame = Gtk::manage(new Gtk::Frame("Robots"));
	robots_frame->add(*Gtk::manage(new RobotsControls(sim)));
	vbox->pack_start(*robots_frame, Gtk::PACK_EXPAND_WIDGET);

	vis_button.set_label("Visualizer");
	vbox->pack_start(vis_button, Gtk::PACK_SHRINK);

	add(*vbox);

	show_all();

	vis_window.set_title("Simulator Visualizer");
	vis_window.add(vis);
	vis_button.signal_toggled().connect(sigc::mem_fun(this, &MainWindow::on_vis_button_toggled));
	vis_window.signal_delete_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::mem_fun(vis_button, &Gtk::ToggleButton::set_active), false), false)));
}

void MainWindow::on_vis_button_toggled() {
	if (vis_button.get_active()) {
		vis_window.show_all();
	} else {
		vis_window.hide_all();
	}
}

