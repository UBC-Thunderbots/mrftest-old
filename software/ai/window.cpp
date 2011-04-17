#include "ai/window.h"
#include "ai/param_panel.h"
#include "uicomponents/abstract_list_model.h"
#include "uicomponents/annunciator.h"
#include "util/algorithm.h"
#include <cassert>
#include <vector>

using AI::Window;

namespace {
	class BasicControls : public Gtk::Frame {
		public:
			BasicControls(AI::AIPackage &ai) : Gtk::Frame("Basics"), ai(ai) {
				Gtk::Table *table = Gtk::manage(new Gtk::Table(1 + ai.backend.main_ui_controls_table_rows(), 3));

				table->attach(*Gtk::manage(new Gtk::Label("Play type:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				playtype_entry.set_editable(false);
				table->attach(playtype_entry, 1, 3, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				ai.backend.playtype().signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_playtype_changed));
				on_playtype_changed();

				ai.backend.main_ui_controls_attach(*table, 1);

				add(*table);
			}

		private:
			AI::AIPackage &ai;
			Gtk::Entry playtype_entry;

			void on_playtype_changed() {
				playtype_entry.set_text(AI::Common::PlayTypeInfo::to_string(ai.backend.playtype()));
			}
	};

	class BallFilterControls : public Gtk::Frame {
		public:
			BallFilterControls(AI::AIPackage &ai) : Gtk::Frame("Ball Filter"), ai(ai) {
				ball_filter_chooser.append_text("<Select Ball Filter>");
				typedef AI::BF::BallFilter::Map Map;
				const Map &m = AI::BF::BallFilter::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					ball_filter_chooser.append_text(i->second->name());
				}
				add(ball_filter_chooser);
				ball_filter_chooser.signal_changed().connect(sigc::mem_fun(this, &BallFilterControls::on_ball_filter_chooser_changed));
				ai.backend.ball_filter().signal_changed().connect(sigc::mem_fun(this, &BallFilterControls::on_ball_filter_changed));
				on_ball_filter_changed();
			}

		private:
			AI::AIPackage &ai;
			Gtk::ComboBoxText ball_filter_chooser;

			void on_ball_filter_chooser_changed() {
				const Glib::ustring &selected = ball_filter_chooser.get_active_text();
				typedef AI::BF::BallFilter::Map Map;
				const Map &m = AI::BF::BallFilter::all();
				const Map::const_iterator &i = m.find(selected.collate_key());
				if (i != m.end()) {
					ai.backend.ball_filter() = i->second;
				} else {
					ai.backend.ball_filter() = 0;
				}
			}

			void on_ball_filter_changed() {
				AI::BF::BallFilter *ball_filter = ai.backend.ball_filter();
				if (ball_filter) {
					ball_filter_chooser.set_active_text(ball_filter->name());
				} else {
					ball_filter_chooser.set_active_text("<Select Ball Filter>");
				}
			}
	};

	class HighLevelControls : public Gtk::Frame {
		public:
			HighLevelControls(AI::AIPackage &ai) : Gtk::Frame("High Level"), ai(ai), table(2, 2), custom_controls(0) {
				high_level_chooser.append_text("<Choose High Level>");
				typedef AI::HL::HighLevelFactory::Map Map;
				const Map &m = AI::HL::HighLevelFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					high_level_chooser.append_text(i->second->name());
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
					ai.high_level = i->second->create_high_level(ai.backend);
				} else {
					ai.high_level = AI::HL::HighLevel::Ptr();
				}
			}

			void on_high_level_changing() {
				if (custom_controls) {
					table.remove(*custom_controls);
					custom_controls = 0;
				}
			}

			void on_high_level_changed() {
				AI::HL::HighLevel::Ptr high_level = ai.high_level;
				if (high_level.is()) {
					high_level_chooser.set_active_text(high_level->factory().name());
				} else {
					high_level_chooser.set_active_text("<Choose High Level>");
				}
				custom_controls = high_level.is() ? high_level->ui_controls() : 0;
				if (custom_controls) {
					table.attach(*custom_controls, 0, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
					custom_controls->show_all();
				}
			}
	};

	class NavigatorControls : public Gtk::Frame {
		public:
			NavigatorControls(AI::AIPackage &ai) : Gtk::Frame("Navigator"), ai(ai), table(3, 2), custom_controls(0) {
				navigator_chooser.append_text("<Choose Navigator>");
				typedef AI::Nav::NavigatorFactory::Map Map;
				const Map &m = AI::Nav::NavigatorFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					navigator_chooser.append_text(i->second->name());
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
					ai.navigator = i->second->create_navigator(ai.backend);
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
				AI::Nav::Navigator::Ptr navigator = ai.navigator;
				if (navigator.is()) {
					navigator_chooser.set_active_text(navigator->factory().name());
				} else {
					navigator_chooser.set_active_text("<Choose Navigator>");
				}
				custom_controls = navigator.is() ? navigator->ui_controls() : 0;
				if (custom_controls) {
					table.attach(*custom_controls, 0, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
					custom_controls->show_all();
				}
			}
	};

	class RobotControllerControls : public Gtk::Frame {
		public:
			RobotControllerControls(AI::AIPackage &ai) : Gtk::Frame("Robot Controller"), ai(ai), table(3, 2), custom_controls(0) {
				rc_chooser.append_text("<Choose Robot Controller>");
				typedef AI::RC::RobotControllerFactory::Map Map;
				const Map &m = AI::RC::RobotControllerFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					rc_chooser.append_text(i->second->name());
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
			SecondaryBasicControls(AI::AIPackage &ai) : Gtk::Table(3 + ai.backend.secondary_ui_controls_table_rows(), 3), ai(ai) {
				attach(*Gtk::manage(new Gtk::Label("Play type override:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				for (unsigned int i = 0; i <= static_cast<unsigned int>(AI::Common::PlayType::NONE); ++i) {
					playtype_override_chooser.append_text(AI::Common::PlayTypeInfo::to_string(AI::Common::PlayTypeInfo::of_int(i)));
				}
				playtype_override_chooser.set_active_text(AI::Common::PlayTypeInfo::to_string(AI::Common::PlayType::NONE));
				attach(playtype_override_chooser, 1, 3, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				playtype_override_chooser.signal_changed().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_playtype_override_chooser_changed));
				ai.backend.playtype_override().signal_changed().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_playtype_override_changed));
				on_playtype_override_changed();

				attach(*Gtk::manage(new Gtk::Label("Defending:")), 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				defending_end_entry.set_editable(false);
				attach(defending_end_entry, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				Gtk::Button *flip_end_button = Gtk::manage(new Gtk::Button("X"));
				attach(*flip_end_button, 2, 3, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				flip_end_button->signal_clicked().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_flip_end_clicked));
				ai.backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_defending_end_changed));
				on_defending_end_changed();

				attach(*Gtk::manage(new Gtk::Label("Colour:")), 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				friendly_colour_entry.set_editable(false);
				attach(friendly_colour_entry, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				Gtk::Button *flip_friendly_colour_button = Gtk::manage(new Gtk::Button("X"));
				attach(*flip_friendly_colour_button, 2, 3, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				flip_friendly_colour_button->signal_clicked().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_flip_friendly_colour_clicked));
				ai.backend.friendly_colour().signal_changed().connect(sigc::mem_fun(this, &SecondaryBasicControls::on_friendly_colour_changed));
				on_friendly_colour_changed();

				ai.backend.secondary_ui_controls_attach(*this, 3);
			}

		private:
			AI::AIPackage &ai;
			Gtk::ComboBoxText playtype_override_chooser;
			Gtk::Entry defending_end_entry, friendly_colour_entry;

			void on_playtype_override_chooser_changed() {
				if (playtype_override_chooser.get_active_row_number() != -1) {
					ai.backend.playtype_override() = AI::Common::PlayTypeInfo::of_int(playtype_override_chooser.get_active_row_number());
				}
			}

			void on_playtype_override_changed() {
				AI::Common::PlayType pt = ai.backend.playtype_override();
				playtype_override_chooser.set_active_text(AI::Common::PlayTypeInfo::to_string(pt));
			}

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
					case AI::Common::Team::Colour::YELLOW:
						ai.backend.friendly_colour() = AI::Common::Team::Colour::BLUE;
						break;

					case AI::Common::Team::Colour::BLUE:
						ai.backend.friendly_colour() = AI::Common::Team::Colour::YELLOW;
						break;
				}
			}

			void on_friendly_colour_changed() {
				friendly_colour_entry.set_text(ai.backend.friendly_colour() == AI::Common::Team::Colour::YELLOW ? "Yellow" : "Blue");
			}
	};

	class VisualizerControls : public Gtk::Table {
		public:
			VisualizerControls(Visualizer &vis) : Gtk::Table(G_N_ELEMENTS(CONTROLS), 2), vis(vis), buttons(G_N_ELEMENTS(CONTROLS), 0) {
				unsigned int children_left = 0;
				for (unsigned int i = 0; i < G_N_ELEMENTS(CONTROLS); ++i) {
					buttons[i] = Gtk::manage(new Gtk::CheckButton(CONTROLS[i].title));
					buttons[i]->set_active(vis.*(CONTROLS[i].flag));
					buttons[i]->signal_toggled().connect(sigc::mem_fun(this, &VisualizerControls::on_toggled));
					if (children_left) {
						attach(*Gtk::manage(new Gtk::Label("    ")), 0, 1, i, i + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
						attach(*buttons[i], 1, 2, i, i + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::SHRINK);
						--children_left;
					} else {
						attach(*buttons[i], 0, 2, i, i + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::SHRINK);
						children_left = CONTROLS[i].num_children;
					}
				}

				on_toggled();
			}

		private:
			struct ControlInfo {
				const char *title;
				bool Visualizer::*flag;
				unsigned int num_children;
			};

			static const ControlInfo CONTROLS[8];

			Visualizer &vis;
			std::vector<Gtk::CheckButton *> buttons;

			void on_toggled() {
				// Update enables.
				unsigned int num_children = 0;
				bool enable = false;
				for (unsigned int i = 0; i < G_N_ELEMENTS(CONTROLS); ++i) {
					if (num_children) {
						buttons[i]->set_sensitive(enable);
						--num_children;
					} else {
						num_children = CONTROLS[i].num_children;
						enable = buttons[i]->get_active();
					}
				}

				// Update visualizer flags.
				for (unsigned int i = 0; i < G_N_ELEMENTS(CONTROLS); ++i) {
					vis.*(CONTROLS[i].flag) = buttons[i]->get_active();
				}
			}
	};

	const VisualizerControls::ControlInfo VisualizerControls::CONTROLS[8] = {
		{ "Field", &Visualizer::show_field, 0 },
		{ "Ball", &Visualizer::show_ball, 1 },
		{ "Velocity", &Visualizer::show_ball_v, 0 },
		{ "Robots", &Visualizer::show_robots, 3 },
		{ "Velocity", &Visualizer::show_robots_v, 0 },
		{ "Destination", &Visualizer::show_robots_dest, 0 },
		{ "Path", &Visualizer::show_robots_path, 0 },
		{ "AI Overlay", &Visualizer::show_overlay, 0 },
	};

	class VisualizerCoordinatesBar : public Gtk::Statusbar {
		public:
			VisualizerCoordinatesBar(Visualizer &vis) {
				vis.signal_mouse_moved().connect(sigc::mem_fun(this, &VisualizerCoordinatesBar::on_move));
			}

		private:
			void on_move(Point p) {
				pop();
				push(Glib::ustring::compose("(%1, %2)", p.x, p.y));
			}
	};
}

Window::Window(AIPackage &ai) {
	Visualizer *visualizer = Gtk::manage(new Visualizer(ai.backend));

	set_title("AI");

	Gtk::VBox *outer_vbox = Gtk::manage(new Gtk::VBox);

	Gtk::HPaned *hpaned = Gtk::manage(new Gtk::HPaned);

	Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook);

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	vbox->pack_start(*Gtk::manage(new BasicControls(ai)), Gtk::PACK_SHRINK);
	vbox->pack_start(*Gtk::manage(new BallFilterControls(ai)), Gtk::PACK_SHRINK);
	vbox->pack_start(*Gtk::manage(new HighLevelControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*Gtk::manage(new NavigatorControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*Gtk::manage(new RobotControllerControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	notebook->append_page(*vbox, "Main");

	vbox = Gtk::manage(new Gtk::VBox);
	Gtk::Frame *frame = Gtk::manage(new Gtk::Frame("Basics"));
	frame->add(*Gtk::manage(new SecondaryBasicControls(ai)));
	vbox->pack_start(*frame, Gtk::PACK_SHRINK);
	frame = Gtk::manage(new Gtk::Frame("Visualizer"));
	frame->add(*Gtk::manage(new VisualizerControls(*visualizer)));
	vbox->pack_start(*frame, Gtk::PACK_SHRINK);
	notebook->append_page(*vbox, "Secondary");

	notebook->append_page(*Gtk::manage(new ParamPanel), "Params");

	frame = Gtk::manage(new Gtk::Frame);
	frame->set_shadow_type(Gtk::SHADOW_IN);
	frame->add(*notebook);
	hpaned->pack1(*frame, false, false);

	Gtk::VPaned *vpaned = Gtk::manage(new Gtk::VPaned);
	vpaned->pack1(*visualizer, true, true);
	vpaned->pack2(*Gtk::manage(new GUIAnnunciator), false, true);

	hpaned->pack2(*vpaned, true, true);

	outer_vbox->pack_start(*hpaned, Gtk::PACK_EXPAND_WIDGET);

	outer_vbox->pack_start(*Gtk::manage(new VisualizerCoordinatesBar(*visualizer)), Gtk::PACK_SHRINK);

	add(*outer_vbox);

	show_all();
}

Window::~Window() {
}

