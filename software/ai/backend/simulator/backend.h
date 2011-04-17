#ifndef AI_BACKEND_SIMULATOR_H
#define AI_BACKEND_SIMULATOR_H

#include "ai/backend/backend.h"
#include "ai/backend/simulator/ball.h"
#include "ai/backend/simulator/field.h"
#include "ai/backend/simulator/player.h"
#include "ai/backend/simulator/robot.h"
#include "ai/backend/simulator/team.h"
#include "simulator/sockproto/proto.h"
#include "util/chdir.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/misc.h"
#include "util/sockaddrs.h"
#include <cerrno>
#include <cstring>
#include <gtkmm.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

namespace AI {
	namespace BE {
		namespace Simulator {
			/**
			 * Packages up the GUI controls that are embedded into the window on the main tab when the simulator backend is used.
			 */
			class MainUIControls {
				public:
					/**
					 * A label for playtype_combo.
					 */
					Gtk::Label playtype_label;

					/**
					 * A combo box for picking the master play type plumbed to both teams.
					 */
					Gtk::ComboBoxText playtype_combo;

					/**
					 * Constructs a new MainUIControls.
					 * The controls are not added to a window yet.
					 */
					MainUIControls();

					/**
					 * Returns the number of rows of controls.
					 *
					 * \return the row count.
					 */
					unsigned int rows() const;

					/**
					 * Attaches the controls into a table.
					 *
					 * \param[in] t the table into which to place the controls.
					 *
					 * \param[in] row the Y coordinate at which the top edge of the first row of components should be placed in the table.
					 */
					void attach(Gtk::Table &t, unsigned int row);
			};

			/**
			 * Packages up the GUI controls that are embedded into the window on the secondary tab when the simulator backend is used.
			 */
			class SecondaryUIControls {
				public:
					/**
					 * A label for speed_hbox.
					 */
					Gtk::Label speed_label;

					/**
					 * A box to contain speed_normal and speed_fast.
					 */
					Gtk::HBox speed_hbox;

					/**
					 * A radio group to contain speed_normal and speed_fast.
					 */
					Gtk::RadioButtonGroup speed_group;

					/**
					 * A radio button to set the simulator to run at normal speed.
					 */
					Gtk::RadioButton speed_normal;

					/**
					 * A radio button to set the simulator to run in fast mode.
					 */
					Gtk::RadioButton speed_fast;

					/**
					 * A radio button to set the simulator to run in slow mode.
					 */
					Gtk::RadioButton speed_slow;

					/**
					 * A label for players_hbox.
					 */
					Gtk::Label players_label;

					/**
					 * A box to contain players_add and players_remove.
					 */
					Gtk::HButtonBox players_hbox;

					/**
					 * A button to add a new player.
					 */
					Gtk::Button players_add;

					/**
					 * A button to remove a player.
					 */
					Gtk::Button players_remove;

					/**
					 * The current state file name.
					 */
					std::string state_file_name;

					/**
					 * A label for state_file_hbox.
					 */
					Gtk::Label state_file_label;

					/**
					 * A display to show the currently-selected state file name.
					 */
					Gtk::Entry state_file_entry;

					/**
					 * A button to choose a state file.
					 */
					Gtk::Button state_file_button;

					/**
					 * A box to contain state_file_load_button and state_file_save_button.
					 */
					Gtk::HButtonBox state_file_hbox;

					/**
					 * A button to load the state file.
					 */
					Gtk::Button state_file_load_button;

					/**
					 * A button to save the state file.
					 */
					Gtk::Button state_file_save_button;

					/**
					 * Constructs a new SecondaryUIControls.
					 * The controls are not added to a window yet.
					 *
					 * \param[in] load_filename the initial state file name.
					 */
					SecondaryUIControls(const std::string &load_filename);

					/**
					 * Returns the number of rows of controls.
					 *
					 * \return the row count.
					 */
					unsigned int rows() const;

					/**
					 * Attaches the controls into a table.
					 *
					 * \param[in] t the table into which to place the controls.
					 *
					 * \param[in] row the Y coordinate at which the top edge of the first row of components should be placed in the table.
					 */
					void attach(Gtk::Table &t, unsigned int row);

				private:
					/**
					 * Pops up a file chooser for the user to choose a state file.
					 */
					void on_state_file_button_clicked();
			};

			/**
			 * Connects to a running simulator.
			 *
			 * \return the connected socket.
			 */
			FileDescriptor::Ptr connect_to_simulator();

			/**
			 * A backend that talks to the simulator process over a UNIX-domain socket.
			 */
			class Backend : public AI::BE::Backend {
				public:
					/**
					 * Fired when a mouse button is pressed over the visualizer.
					 */
					mutable sigc::signal<void, Point, unsigned int> signal_mouse_pressed;

					/**
					 * Fired when a mouse button is released over the visualizer.
					 */
					mutable sigc::signal<void, Point, unsigned int> signal_mouse_released;

					/**
					 * Fired when the mouse pointer leaves the visualizer area.
					 */
					mutable sigc::signal<void> signal_mouse_exited;

					/**
					 * Fired when the mouse moves within the visualizer area.
					 */
					mutable sigc::signal<void, Point> signal_mouse_moved;

					/**
					 * Constructs a new Backend and connects to the running simulator.
					 *
					 * \param[in] load_filename the name of a state file to load.
					 */
					Backend(const std::string &load_filename);

					/**
					 * Sends a packet to the simulator.
					 *
					 * \param[in] packet the packet to send.
					 *
					 * \param[in] ancillary_fd the ancillary file descriptor to send with the message.
					 */
					void send_packet(const ::Simulator::Proto::A2SPacket &packet, FileDescriptor::Ptr ancillary_fd = FileDescriptor::Ptr());

					AI::BE::BackendFactory &factory() const;
					const AI::BE::Simulator::Field &field() const;
					const AI::BE::Simulator::Ball &ball() const;
					AI::BE::Simulator::FriendlyTeam &friendly_team();
					const AI::BE::Simulator::FriendlyTeam &friendly_team() const;
					const AI::BE::Simulator::EnemyTeam &enemy_team() const;
					timespec monotonic_time() const;
					std::size_t visualizable_num_robots() const;
					Visualizable::Robot::Ptr visualizable_robot(std::size_t i) const;
					void mouse_pressed(Point p, unsigned int btn);
					void mouse_released(Point p, unsigned int btn);
					void mouse_exited();
					void mouse_moved(Point p);
					unsigned int main_ui_controls_table_rows() const;
					void main_ui_controls_attach(Gtk::Table &t, unsigned int row);
					unsigned int secondary_ui_controls_table_rows() const;
					void secondary_ui_controls_attach(Gtk::Table &t, unsigned int row);

				private:
					/**
					 * The UNIX-domain socket connected to the server.
					 */
					FileDescriptor::Ptr sock;

					/**
					 * The field geometry.
					 */
					AI::BE::Simulator::Field field_;

					/**
					 * The ball.
					 */
					AI::BE::Simulator::Ball ball_;

					/**
					 * The friendly team.
					 */
					AI::BE::Simulator::FriendlyTeam friendly_;

					/**
					 * The enemy team.
					 */
					AI::BE::Simulator::EnemyTeam enemy_;

					/**
					 * The timestamp in game monotonic time of the most recent AI tick.
					 */
					timespec monotonic_time_;

					/**
					 * The current master play type.
					 */
					AI::Common::PlayType simulator_playtype;

					/**
					 * The main tab UI controls.
					 */
					MainUIControls main_controls;

					/**
					 * The secondary tab UI controls.
					 */
					SecondaryUIControls secondary_controls;

					/**
					 * Invoked when a packet is ready to receive from the simulator over the socket.
					 *
					 * \return \c true, to continue accepting more packets.
					 */
					bool on_packet(Glib::IOCondition);

					/**
					 * Sends a set of robot orders to the simulator.
					 */
					void send_orders();

					/**
					 * Examines the current master play type and play type override and calculates the effective play type.
					 */
					void update_playtype();

					/**
					 * Invoked when the user selects a new value in the master play type combobox.
					 * Sends the new selected play type to the simulator.
					 */
					void on_sim_playtype_changed();

					/**
					 * Invoked when the user selects a new speed mode radio button.
					 * Sends the new selected speed mode to the simulator.
					 */
					void on_speed_toggled();

					/**
					 * Invoked when the user clicks the add player button.
					 * Sends the request to the simulator and prevents further team edits until the next tick.
					 */
					void on_players_add_clicked();

					/**
					 * Invoked when the user clicks the remove player button.
					 * Sends the request to the simulator and prevents further team edits until the next tick.
					 */
					void on_players_remove_clicked();

					/**
					 * Checks whether a particular lid pattern exists in the friendly team.
					 *
					 * \param[in] pattern the pattern to look for.
					 *
					 * \return \c true if \p pattern exists, or \c false if not.
					 */
					bool pattern_exists(unsigned int pattern);

					/**
					 * Handles a request to load the current state file.
					 */
					void on_state_file_load_clicked();

					/**
					 * Handles a request to save the current state file.
					 */
					void on_state_file_save_clicked();
			};

			/**
			 * A factory for creating \ref Backend "Backends".
			 */
			class BackendFactory : public AI::BE::BackendFactory {
				public:
					/**
					 * Constructs a new BackendFactory.
					 */
					BackendFactory();

					void create_backend(const std::multimap<Glib::ustring, Glib::ustring> &params, std::function<void(AI::BE::Backend &)> cb) const;
			};
		}
	}
}

#endif

