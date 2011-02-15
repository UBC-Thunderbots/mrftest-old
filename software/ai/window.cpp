#include "ai/window.h"
#include "uicomponents/abstract_list_model.h"
#include "uicomponents/annunciator.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include <cassert>

using AI::Window;

namespace {
	class BasicControls : public Gtk::Frame {
		public:
			BasicControls(AI::AIPackage &ai) : Gtk::Frame("Basics"), ai(ai) {
				Gtk::Table *table = Gtk::manage(new Gtk::Table(4 + ai.backend.ui_controls_table_rows(), 3));

				table->attach(*Gtk::manage(new Gtk::Label("Play type override:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				playtype_override_chooser.append_text("<None>");
				for (unsigned int i = 0; i < AI::Common::PlayType::COUNT; ++i) {
					playtype_override_chooser.append_text(AI::Common::PlayType::DESCRIPTIONS_GENERIC[i]);
				}
				table->attach(playtype_override_chooser, 1, 3, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				playtype_override_chooser.signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_playtype_override_chooser_changed));
				ai.backend.playtype_override().signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_playtype_override_changed));
				on_playtype_override_changed();

				table->attach(*Gtk::manage(new Gtk::Label("Play type:")), 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				playtype_entry.set_editable(false);
				table->attach(playtype_entry, 1, 3, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				ai.backend.playtype().signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_playtype_changed));
				on_playtype_changed();

				table->attach(*Gtk::manage(new Gtk::Label("Defending:")), 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				defending_end_entry.set_editable(false);
				table->attach(defending_end_entry, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				Gtk::Button *flip_end_button = Gtk::manage(new Gtk::Button("X"));
				table->attach(*flip_end_button, 2, 3, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				flip_end_button->signal_clicked().connect(sigc::mem_fun(this, &BasicControls::on_flip_end_clicked));
				ai.backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_defending_end_changed));
				on_defending_end_changed();

				table->attach(*Gtk::manage(new Gtk::Label("Colour:")), 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				friendly_colour_entry.set_editable(false);
				table->attach(friendly_colour_entry, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				Gtk::Button *flip_friendly_colour_button = Gtk::manage(new Gtk::Button("X"));
				table->attach(*flip_friendly_colour_button, 2, 3, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				flip_friendly_colour_button->signal_clicked().connect(sigc::mem_fun(this, &BasicControls::on_flip_friendly_colour_clicked));
				ai.backend.friendly_colour().signal_changed().connect(sigc::mem_fun(this, &BasicControls::on_friendly_colour_changed));
				on_friendly_colour_changed();

				ai.backend.ui_controls_attach(*table, 4);

				add(*table);
			}

		private:
			AI::AIPackage &ai;
			Gtk::ComboBoxText playtype_override_chooser;
			Gtk::Entry playtype_entry, defending_end_entry, friendly_colour_entry;

			void on_playtype_override_chooser_changed() {
				const Glib::ustring &selected = playtype_override_chooser.get_active_text();
				for (unsigned int i = 0; i < AI::Common::PlayType::COUNT; ++i) {
					if (selected == AI::Common::PlayType::DESCRIPTIONS_GENERIC[i]) {
						ai.backend.playtype_override() = static_cast<AI::Common::PlayType::PlayType>(i);
						return;
					}
				}
				ai.backend.playtype_override() = AI::Common::PlayType::COUNT;
			}

			void on_playtype_override_changed() {
				AI::Common::PlayType::PlayType pt = ai.backend.playtype_override();
				if (pt < AI::Common::PlayType::COUNT) {
					playtype_override_chooser.set_active_text(AI::Common::PlayType::DESCRIPTIONS_GENERIC[pt]);
				} else {
					playtype_override_chooser.set_active_text("<None>");
				}
			}

			void on_playtype_changed() {
				playtype_entry.set_text(AI::Common::PlayType::DESCRIPTIONS_GENERIC[ai.backend.playtype()]);
			}

			void on_flip_end_clicked() {
				ai.backend.defending_end() = static_cast<AI::BE::Backend::FieldEnd>((ai.backend.defending_end() + 1) % 2);
			}

			void on_defending_end_changed() {
				assert(ai.backend.defending_end() == AI::BE::Backend::WEST || ai.backend.defending_end() == AI::BE::Backend::EAST);
				defending_end_entry.set_text(ai.backend.defending_end() == AI::BE::Backend::WEST ? "West" : "East");
			}

			void on_flip_friendly_colour_clicked() {
				ai.backend.friendly_colour() = static_cast<AI::BE::Backend::Colour>((ai.backend.friendly_colour() + 1) % 2);
			}

			void on_friendly_colour_changed() {
				assert(ai.backend.friendly_colour() == AI::BE::Backend::YELLOW || ai.backend.friendly_colour() == AI::BE::Backend::BLUE);
				friendly_colour_entry.set_text(ai.backend.friendly_colour() == AI::BE::Backend::YELLOW ? "Yellow" : "Blue");
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
				table.attach(*Gtk::manage(new Gtk::Label("High Level:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				high_level_chooser.append_text("<Choose High Level>");
				typedef AI::HL::HighLevelFactory::Map Map;
				const Map &m = AI::HL::HighLevelFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					high_level_chooser.append_text(i->second->name());
				}
				table.attach(high_level_chooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
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
				table.attach(*Gtk::manage(new Gtk::Label("Navigator:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				navigator_chooser.append_text("<Choose Navigator>");
				typedef AI::Nav::NavigatorFactory::Map Map;
				const Map &m = AI::Nav::NavigatorFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					navigator_chooser.append_text(i->second->name());
				}

				table.attach(navigator_chooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
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
				table.attach(*Gtk::manage(new Gtk::Label("Robot Controller:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				rc_chooser.append_text("<Choose Robot Controller>");
				typedef AI::RC::RobotControllerFactory::Map Map;
				const Map &m = AI::RC::RobotControllerFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					rc_chooser.append_text(i->second->name());
				}
				table.attach(rc_chooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
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

	class VisualizerCoordinatesBar : public Gtk::Statusbar {
		public:
			VisualizerCoordinatesBar(Visualizer &vis) {
				vis.signal_mouse_moved().connect(sigc::mem_fun(this, &VisualizerCoordinatesBar::on_move));
			}

			~VisualizerCoordinatesBar() {
			}

		private:
			void on_move(Point p) {
				pop();
				push(Glib::ustring::compose("(%1, %2)", p.x, p.y));
			}
	};
}

Window::Window(AIPackage &ai) {
	set_title("AI");

	Gtk::VBox *outer_vbox = Gtk::manage(new Gtk::VBox);

	Gtk::HPaned *hpaned = Gtk::manage(new Gtk::HPaned);

	Gtk::Frame *frame = Gtk::manage(new Gtk::Frame);
	frame->set_shadow_type(Gtk::SHADOW_IN);

	Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook);

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	vbox->pack_start(*Gtk::manage(new BasicControls(ai)), Gtk::PACK_SHRINK);
	vbox->pack_start(*Gtk::manage(new BallFilterControls(ai)), Gtk::PACK_SHRINK);
	vbox->pack_start(*Gtk::manage(new HighLevelControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*Gtk::manage(new NavigatorControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*Gtk::manage(new RobotControllerControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*Gtk::manage(new GUIAnnunciator), Gtk::PACK_EXPAND_WIDGET);

	notebook->append_page(*vbox, "Main");

	notebook->append_page(*Gtk::manage(new ParamPanel), "Params");

	frame->add(*notebook);

	hpaned->pack1(*frame, Gtk::FILL);

	frame = Gtk::manage(new Gtk::Frame);
	frame->set_shadow_type(Gtk::SHADOW_IN);
	Visualizer *visualizer = Gtk::manage(new Visualizer(ai.backend));
	frame->add(*visualizer);

	hpaned->pack2(*frame, Gtk::EXPAND | Gtk::FILL);

	outer_vbox->pack_start(*hpaned, Gtk::PACK_EXPAND_WIDGET);

	outer_vbox->pack_start(*Gtk::manage(new VisualizerCoordinatesBar(*visualizer)), Gtk::PACK_SHRINK);

	add(*outer_vbox);

	show_all();
}

Window::~Window() {
}

