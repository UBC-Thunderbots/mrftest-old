#include "ai/window.h"
#include "uicomponents/abstract_list_model.h"
#include "uicomponents/annunciator.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include <cassert>
#include <cstdlib>
#include <iomanip>

using AI::Window;

namespace {
	class BasicControls : public Gtk::Frame {
		public:
			BasicControls(AI::AIPackage &ai) : Gtk::Frame("Basics"), ai(ai) {
				Gtk::Table *table = Gtk::manage(new Gtk::Table(4, 3));

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
					ball_filter_chooser.append_text(i->second->name);
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
					ball_filter_chooser.set_active_text(ball_filter->name);
				} else {
					ball_filter_chooser.set_active_text("<Select Ball Filter>");
				}
			}
	};

	class CoachControls : public Gtk::Frame {
		public:
			CoachControls(AI::AIPackage &ai) : Gtk::Frame("Coach"), ai(ai) {
				Gtk::Table *table = Gtk::manage(new Gtk::Table(2, 2));

				table->attach(*Gtk::manage(new Gtk::Label("Coach:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				coach_chooser.append_text("<Choose Coach>");
				typedef AI::Coach::CoachFactory::Map Map;
				const Map &m = AI::Coach::CoachFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					coach_chooser.append_text(i->second->name);
				}
				table->attach(coach_chooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				coach_chooser.signal_changed().connect(sigc::mem_fun(this, &CoachControls::on_coach_chooser_changed));
				ai.coach.signal_changed().connect(sigc::mem_fun(this, &CoachControls::on_coach_changed));
				on_coach_changed();

				table->attach(*Gtk::manage(new Gtk::Label("Strategy:")), 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				strategy_entry.set_editable(false);
				table->attach(strategy_entry, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				ai.backend.strategy().signal_changed().connect(sigc::mem_fun(this, &CoachControls::on_strategy_changed));
				on_strategy_changed();
			}

		private:
			AI::AIPackage &ai;
			Gtk::ComboBoxText coach_chooser;
			Gtk::Entry strategy_entry;

			void on_coach_chooser_changed() {
				const Glib::ustring &selected = coach_chooser.get_active_text();
				typedef AI::Coach::CoachFactory::Map Map;
				const Map &m = AI::Coach::CoachFactory::all();
				const Map::const_iterator &i = m.find(selected.collate_key());
				if (i != m.end()) {
					ai.coach = i->second->create_coach(ai.backend);
				} else {
					ai.coach = AI::Coach::Coach::Ptr();
				}
			}

			void on_coach_changed() {
				AI::Coach::Coach::Ptr coach = ai.coach;
				if (coach.is()) {
					coach_chooser.set_active_text(coach->factory().name);
				} else {
					coach_chooser.set_active_text("<Choose Coach>");
				}
			}

			void on_strategy_changed() {
				AI::HL::Strategy::Ptr strategy = ai.backend.strategy();
				if (strategy.is()) {
					strategy_entry.set_text(strategy->factory().name);
				} else {
					strategy_entry.set_text("<None>");
				}
			}
	};

	class NavigatorControls : public Gtk::Frame {
		public:
			NavigatorControls(AI::AIPackage &ai) : Gtk::Frame("Navigator"), ai(ai) {
				Gtk::Table *table = Gtk::manage(new Gtk::Table(2, 2));

				table->attach(*Gtk::manage(new Gtk::Label("Navigator:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				navigator_chooser.append_text("<Choose Navigator>");
				typedef AI::Nav::NavigatorFactory::Map Map;
				const Map &m = AI::Nav::NavigatorFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					navigator_chooser.append_text(i->second->name);
				}
				table->attach(navigator_chooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				navigator_chooser.signal_changed().connect(sigc::mem_fun(this, &NavigatorControls::on_navigator_chooser_changed));
				ai.navigator.signal_changed().connect(sigc::mem_fun(this, &NavigatorControls::on_navigator_changed));
				on_navigator_changed();
			}

		private:
			AI::AIPackage &ai;
			Gtk::ComboBoxText navigator_chooser;

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

			void on_navigator_changed() {
				AI::Nav::Navigator::Ptr navigator = ai.navigator;
				if (navigator.is()) {
					navigator_chooser.set_active_text(navigator->factory().name);
				} else {
					navigator_chooser.set_active_text("<Choose Navigator>");
				}
			}
	};

	class RobotControllerControls : public Gtk::Frame {
		public:
			RobotControllerControls(AI::AIPackage &ai) : Gtk::Frame("Robot Controller"), ai(ai) {
				Gtk::Table *table = Gtk::manage(new Gtk::Table(2, 2));

				table->attach(*Gtk::manage(new Gtk::Label("Robot Controller:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				rc_chooser.append_text("<Choose Robot Controller>");
				typedef AI::RC::RobotControllerFactory::Map Map;
				const Map &m = AI::RC::RobotControllerFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					rc_chooser.append_text(i->second->name);
				}
				table->attach(rc_chooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				rc_chooser.signal_changed().connect(sigc::mem_fun(this, &RobotControllerControls::on_rc_chooser_changed));
				ai.robot_controller_factory.signal_changed().connect(sigc::mem_fun(this, &RobotControllerControls::on_rc_changed));
				on_rc_changed();
			}

		private:
			AI::AIPackage &ai;
			Gtk::ComboBoxText rc_chooser;

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

			void on_rc_changed() {
				AI::RC::RobotControllerFactory *rcf = ai.robot_controller_factory;
				if (rcf) {
					rc_chooser.set_active_text(rcf->name);
				} else {
					rc_chooser.set_active_text("<Choose Robot Controller>");
				}
			}
	};
}

Window::Window(AIPackage &ai) {
	set_title("AI");

	Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook);

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	vbox->pack_start(*Gtk::manage(new BasicControls(ai)), Gtk::PACK_SHRINK);
	vbox->pack_start(*Gtk::manage(new BallFilterControls(ai)), Gtk::PACK_SHRINK);
	vbox->pack_start(*Gtk::manage(new CoachControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*Gtk::manage(new NavigatorControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*Gtk::manage(new RobotControllerControls(ai)), Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*Gtk::manage(new Annunciator), Gtk::PACK_EXPAND_WIDGET);

	notebook->append_page(*vbox, "Main");

	notebook->append_page(*Gtk::manage(new ParamPanel), "Params");

	add(*notebook);

	show_all();
}

Window::~Window() {
}

