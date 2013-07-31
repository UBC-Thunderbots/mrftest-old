#include "ai/window.h"
#include "uicomponents/abstract_list_model.h"
#include "util/algorithm.h"
#include <cassert>
#include <cstdlib>
#include <vector>
#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/table.h>

using AI::Window;

namespace {
	class BasicControls : public Gtk::Frame {
		public:
			explicit BasicControls(AI::AIPackage &ai) : Gtk::Frame("Basics"), ai(ai), table(2 + ai.backend.main_ui_controls_table_rows(), 3), playtype_override_label("Play type override:"), playtype_label("Play type:") {
				table.attach(playtype_override_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				playtype_override_chooser.append_text(AI::Common::PlayTypeInfo::to_string(AI::Common::PlayType::NONE));
				for (unsigned int i = 0; i < static_cast<unsigned int>(AI::Common::PlayType::NONE); ++i) {
					playtype_override_chooser.append_text(AI::Common::PlayTypeInfo::to_string(AI::Common::PlayTypeInfo::of_int(i)));
				}
				playtype_override_chooser.set_active_text(AI::Common::PlayTypeInfo::to_string(AI::Common::PlayType::NONE));
				table.attach(playtype_override_chooser, 1, 3, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				playtype_override_chooser.signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_playtype_override_chooser_changed));
				ai.backend.playtype_override().signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_playtype_override_changed));
				on_playtype_override_changed();

				table.attach(playtype_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				playtype_entry.set_editable(false);
				table.attach(playtype_entry, 1, 3, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				ai.backend.playtype().signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_playtype_changed));
				on_playtype_changed();

				ai.backend.main_ui_controls_attach(table, 2);

				add(table);
			}

		private:
			AI::AIPackage &ai;
			Gtk::Table table;
			Gtk::Label playtype_override_label, playtype_label;
			Gtk::ComboBoxText playtype_override_chooser;
			Gtk::Entry playtype_entry;

			void on_playtype_override_chooser_changed() {
				int row = playtype_override_chooser.get_active_row_number();
				if (row == 0) {
					ai.backend.playtype_override() = AI::Common::PlayType::NONE;
				} else if (row > 0) {
					ai.backend.playtype_override() = AI::Common::PlayTypeInfo::of_int(static_cast<unsigned int>(row) - 1);
				}
			}

			void on_playtype_override_changed() {
				AI::Common::PlayType pt = ai.backend.playtype_override();
				playtype_override_chooser.set_active_text(AI::Common::PlayTypeInfo::to_string(pt));
			}

			void on_playtype_changed() {
				playtype_entry.set_text(AI::Common::PlayTypeInfo::to_string(ai.backend.playtype()));
			}
	};

	class HighLevelControls : public Gtk::Frame {
		public:
			explicit HighLevelControls(AI::AIPackage &ai) : Gtk::Frame("High Level"), ai(ai), table(2, 2), custom_controls(0) {
				high_level_chooser.append_text("<Choose High Level>");
				typedef AI::HL::HighLevelFactory::Map Map;
				for (const Map::value_type &i : AI::HL::HighLevelFactory::all()) {
					high_level_chooser.append_text(i.second->name());
				}
				table.attach(high_level_chooser, 0, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				high_level_chooser.signal_changed().connect(sigc::mem_fun(this, &HighLevelControls::on_high_level_chooser_changed));
				ai.high_level.signal_changing().connect(sigc::mem_fun(this, &HighLevelControls::on_high_level_changing));
				ai.high_level.signal_changed().connect(sigc::mem_fun(this, &HighLevelControls::on_high_level_changed));

				add(table);

				on_high_level_changed();
			}

		private:
			AI::AIPackage &ai;
			Gtk::Table table;
			Gtk::ComboBoxText high_level_chooser;
			Gtk::Widget *custom_controls;

			void on_high_level_chooser_changed() {
				const Glib::ustring &selected = high_level_chooser.get_active_text();
				typedef AI::HL::HighLevelFactory::Map Map;
				const Map &m = AI::HL::HighLevelFactory::all();
				const Map::const_iterator &i = m.find(selected.collate_key());
				if (i != m.end()) {
					if (!ai.high_level.get() || &ai.high_level->factory() != i->second) {
						ai.high_level = i->second->create_high_level(AI::HL::W::World(ai.backend));
					}
				} else {
					ai.high_level = std::unique_ptr<AI::HL::HighLevel>();
				}
			}

			void on_high_level_changing() {
				if (custom_controls) {
					table.remove(*custom_controls);
					custom_controls = 0;
				}
			}

			void on_high_level_changed() {
				if (ai.high_level.get()) {
					high_level_chooser.set_active_text(ai.high_level->factory().name());
				} else {
					high_level_chooser.set_active_text("<Choose High Level>");
				}
				custom_controls = ai.high_level.get() ? ai.high_level->ui_controls() : 0;
				if (custom_controls) {
					table.attach(*custom_controls, 0, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
					custom_controls->show_all();
				}
			}
	};

	class NavigatorControls : public Gtk::Frame {
		public:
			explicit NavigatorControls(AI::AIPackage &ai) : Gtk::Frame("Navigator"), ai(ai), table(3, 2), custom_controls(0) {
				navigator_chooser.append_text("<Choose Navigator>");
				typedef AI::Nav::NavigatorFactory::Map Map;
				for (const Map::value_type &i : AI::Nav::NavigatorFactory::all()) {
					navigator_chooser.append_text(i.second->name());
				}

				table.attach(navigator_chooser, 0, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				navigator_chooser.signal_changed().connect(sigc::mem_fun(this, &NavigatorControls::on_navigator_chooser_changed));
				ai.navigator.signal_changing().connect(sigc::mem_fun(this, &NavigatorControls::on_navigator_changing));
				ai.navigator.signal_changed().connect(sigc::mem_fun(this, &NavigatorControls::on_navigator_changed));

				on_navigator_changed();

				add(table);
			}

		private:
			AI::AIPackage &ai;
			Gtk::Table table;
			Gtk::ComboBoxText navigator_chooser;
			Gtk::Widget *custom_controls;

			void on_navigator_chooser_changed() {
				const Glib::ustring &selected = navigator_chooser.get_active_text();
				typedef AI::Nav::NavigatorFactory::Map Map;
				const Map &m = AI::Nav::NavigatorFactory::all();
				const Map::const_iterator &i = m.find(selected.collate_key());
				if (i != m.end()) {
					if (!ai.navigator.get() || &ai.navigator->factory() != i->second) {
						ai.navigator = i->second->create_navigator(AI::Nav::W::World(ai.backend));
					}
				} else {
					ai.navigator = AI::Nav::Navigator::Ptr();
				}
			}

			void on_navigator_changing() {
				if (custom_controls) {
					table.remove(*custom_controls);
					custom_controls = 0;
				}
			}

			void on_navigator_changed() {
				if (ai.navigator.get()) {
					navigator_chooser.set_active_text(ai.navigator->factory().name());
				} else {
					navigator_chooser.set_active_text("<Choose Navigator>");
				}
				custom_controls = ai.navigator.get() ? ai.navigator->ui_controls() : 0;
				if (custom_controls) {
					table.attach(*custom_controls, 0, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
					custom_controls->show_all();
				}
			}
	};

	class RobotControllerControls : public Gtk::Frame {
		public:
			explicit RobotControllerControls(AI::AIPackage &ai) : Gtk::Frame("Robot Controller"), ai(ai), table(3, 2), custom_controls(0) {
				rc_chooser.append_text("<Choose Robot Controller>");
				typedef AI::RC::RobotControllerFactory::Map Map;
				for (const Map::value_type &i : AI::RC::RobotControllerFactory::all()) {
					rc_chooser.append_text(i.second->name());
				}
				table.attach(rc_chooser, 0, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				rc_chooser.signal_changed().connect(sigc::mem_fun(this, &RobotControllerControls::on_rc_chooser_changed));
				ai.robot_controller_factory.signal_changed().connect(sigc::mem_fun(this, &RobotControllerControls::on_rc_changing));
				ai.robot_controller_factory.signal_changed().connect(sigc::mem_fun(this, &RobotControllerControls::on_rc_changed));

				on_rc_changed();

				add(table);
			}

		private:
			AI::AIPackage &ai;
			Gtk::Table table;
			Gtk::ComboBoxText rc_chooser;
			Gtk::Widget *custom_controls;

			void on_rc_chooser_changed() {
				const Glib::ustring &selected = rc_chooser.get_active_text();
				typedef AI::RC::RobotControllerFactory::Map Map;
				const Map &m = AI::RC::RobotControllerFactory::all();
				const Map::const_iterator &i = m.find(selected.collate_key());
				if (i != m.end()) {
					ai.robot_controller_factory = i->second;
				} else {
					ai.robot_controller_factory = 0;
				}
			}

			void on_rc_changing() {
				if (custom_controls) {
					table.remove(*custom_controls);
					custom_controls = 0;
				}
			}

			void on_rc_changed() {
				AI::RC::RobotControllerFactory *rcf = ai.robot_controller_factory;
				if (rcf) {
					rc_chooser.set_active_text(rcf->name());
				} else {
					rc_chooser.set_active_text("<Choose Robot Controller>");
				}
				custom_controls = rcf ? rcf->ui_controls() : 0;
				if (custom_controls) {
					table.attach(*custom_controls, 0, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
					custom_controls->show_all();
				}
			}
	};

	class SecondaryBasicControls : public Gtk::Table {
		public:
			explicit SecondaryBasicControls(AI::AIPackage &ai) : Gtk::Table(2 + ai.backend.secondary_ui_controls_table_rows(), 3), ai(ai), defending_end_label("Defending:"), friendly_colour_label("Colour:"), flip_end_button("X"), flip_friendly_colour_button("X") {
				attach(defending_end_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				defending_end_entry.set_editable(false);
				attach(defending_end_entry, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				attach(flip_end_button, 2, 3, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				flip_end_button.signal_clicked().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_flip_end_clicked));
				ai.backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_defending_end_changed));
				on_defending_end_changed();

				attach(friendly_colour_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				friendly_colour_entry.set_editable(false);
				attach(friendly_colour_entry, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				attach(flip_friendly_colour_button, 2, 3, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				flip_friendly_colour_button.signal_clicked().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_flip_friendly_colour_clicked));
				ai.backend.friendly_colour().signal_changed().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_friendly_colour_changed));
				on_friendly_colour_changed();

				ai.backend.secondary_ui_controls_attach(*this, 2);
			}

		private:
			AI::AIPackage &ai;
			Gtk::Label defending_end_label, friendly_colour_label;
			Gtk::Entry defending_end_entry, friendly_colour_entry;
			Gtk::Button flip_end_button, flip_friendly_colour_button;

			void on_flip_end_clicked() {
				switch (ai.backend.defending_end()) {
					case AI::BE::Backend::FieldEnd::EAST:
						ai.backend.defending_end() = AI::BE::Backend::FieldEnd::WEST;
						break;

					case AI::BE::Backend::FieldEnd::WEST:
						ai.backend.defending_end() = AI::BE::Backend::FieldEnd::EAST;
						break;
				}
			}

			void on_defending_end_changed() {
				defending_end_entry.set_text(ai.backend.defending_end() == AI::BE::Backend::FieldEnd::WEST ? "West" : "East");
			}

			void on_flip_friendly_colour_clicked() {
				switch (ai.backend.friendly_colour()) {
					case AI::Common::Colour::YELLOW:
						ai.backend.friendly_colour() = AI::Common::Colour::BLUE;
						break;

					case AI::Common::Colour::BLUE:
						ai.backend.friendly_colour() = AI::Common::Colour::YELLOW;
						break;
				}
			}

			void on_friendly_colour_changed() {
				friendly_colour_entry.set_text(ai.backend.friendly_colour() == AI::Common::Colour::YELLOW ? "Yellow" : "Blue");
			}
	};

	class VisualizerControls : public Gtk::Table {
		public:
			explicit VisualizerControls(AI::AIPackage &ai, Visualizer &vis) : Gtk::Table(G_N_ELEMENTS(CONTROLS), 2), ai(ai), vis(vis) {
				unsigned int children_left = 0;
				for (unsigned int i = 0; i < G_N_ELEMENTS(CONTROLS); ++i) {
					buttons[i].set_label(CONTROLS[i].title);
					if (CONTROLS[i].vis_flag) {
						buttons[i].set_active(vis.*(CONTROLS[i].vis_flag));
					} else if (CONTROLS[i].ai_flag) {
						buttons[i].set_active(ai.*(CONTROLS[i].ai_flag));
					} else {
						std::abort();
					}
					buttons[i].signal_toggled().connect(sigc::mem_fun(this, &VisualizerControls::on_toggled));
					if (children_left) {
						spacers[i].set_text("    ");
						attach(spacers[i], 0, 1, i, i + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
						attach(buttons[i], 1, 2, i, i + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::SHRINK);
						--children_left;
					} else {
						attach(buttons[i], 0, 2, i, i + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::SHRINK);
						children_left = CONTROLS[i].num_children;
					}
				}

				on_toggled();
			}

		private:
			struct ControlInfo {
				const char *title;
				bool Visualizer::*vis_flag;
				bool AI::AIPackage::*ai_flag;
				unsigned int num_children;
			};

			static const ControlInfo CONTROLS[12];

			AI::AIPackage &ai;
			Visualizer &vis;
			Gtk::Label spacers[G_N_ELEMENTS(CONTROLS)];
			Gtk::CheckButton buttons[G_N_ELEMENTS(CONTROLS)];

			void on_toggled() {
				// Update enables.
				unsigned int num_children = 0;
				bool enable = false;
				for (unsigned int i = 0; i < G_N_ELEMENTS(CONTROLS); ++i) {
					if (num_children) {
						buttons[i].set_sensitive(enable);
						--num_children;
					} else {
						num_children = CONTROLS[i].num_children;
						enable = buttons[i].get_active();
					}
				}

				// Update visualizer flags.
				for (unsigned int i = 0; i < G_N_ELEMENTS(CONTROLS); ++i) {
					if (CONTROLS[i].vis_flag) {
						vis.*(CONTROLS[i].vis_flag) = buttons[i].get_active();
					}
					if (CONTROLS[i].ai_flag) {
						ai.*(CONTROLS[i].ai_flag) = buttons[i].get_active();
					}
				}
			}
	};

	const VisualizerControls::ControlInfo VisualizerControls::CONTROLS[12] = {
		{ "Field", &Visualizer::show_field, 0, 0 },
		{ "Ball", &Visualizer::show_ball, 0, 1 },
		{ "Velocity", &Visualizer::show_ball_v, 0, 0 },
		{ "Robots", &Visualizer::show_robots, 0, 4 },
		{ "Velocity", &Visualizer::show_robots_v, 0, 0 },
		{ "Destination", &Visualizer::show_robots_dest, 0, 0 },
		{ "Path", &Visualizer::show_robots_path, 0, 0 },
		{ "Graphs", &Visualizer::show_robots_graphs, 0, 0 },
		{ "AI Overlays", &Visualizer::show_overlay, 0, 3 },
		{ "High-Level", 0, &AI::AIPackage::show_hl_overlay, 0 },
		{ "Navigator", 0, &AI::AIPackage::show_nav_overlay, 0 },
		{ "Robot Controller", 0, &AI::AIPackage::show_rc_overlay, 0 },
	};

	class VisualizerCoordinatesBar : public Gtk::Statusbar {
		public:
			explicit VisualizerCoordinatesBar(Visualizer &vis) {
				vis.signal_mouse_moved().connect(sigc::mem_fun(this, &VisualizerCoordinatesBar::on_move));
			}

		private:
			void on_move(Point p) {
				pop();
				push(Glib::ustring::compose("(%1, %2)", p.x, p.y));
			}
	};
}

Window::Window(AIPackage &ai) : visualizer(ai.backend) {
	set_title("AI");

	main_vbox.pack_start(*Gtk::manage(new BasicControls(ai)), Gtk::PACK_SHRINK);
	main_vbox.pack_start(*Gtk::manage(new HighLevelControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	main_vbox.pack_start(*Gtk::manage(new NavigatorControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	main_vbox.pack_start(*Gtk::manage(new RobotControllerControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	notebook.append_page(main_vbox, "Main");

	secondary_basics_frame.set_label("Basics");
	secondary_basics_frame.add(*Gtk::manage(new SecondaryBasicControls(ai)));
	secondary_vbox.pack_start(secondary_basics_frame, Gtk::PACK_SHRINK);
	secondary_visualizer_controls_frame.set_label("Visualizer");
	secondary_visualizer_controls_frame.add(*Gtk::manage(new VisualizerControls(ai, visualizer)));
	secondary_vbox.pack_start(secondary_visualizer_controls_frame, Gtk::PACK_SHRINK);
	notebook.append_page(secondary_vbox, "Secondary");

	notebook.append_page(param_panel, "Params");

	notebook_frame.set_shadow_type(Gtk::SHADOW_IN);
	notebook_frame.add(notebook);
	hpaned.pack1(notebook_frame, false, false);

	vpaned.pack1(visualizer, true, true);
	vpaned.pack2(annunciator, false, true);

	hpaned.pack2(vpaned, true, true);

	outer_vbox.pack_start(hpaned, Gtk::PACK_EXPAND_WIDGET);

	outer_vbox.pack_start(*Gtk::manage(new VisualizerCoordinatesBar(visualizer)), Gtk::PACK_SHRINK);

	add(outer_vbox);

	show_all();
}

