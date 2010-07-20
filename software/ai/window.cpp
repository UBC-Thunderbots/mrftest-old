#include "ai/window.h"
#include "uicomponents/abstract_list_model.h"
#include "uicomponents/annunciator.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include <cstdlib>
#include <iomanip>

namespace {
	/**
	 * A list model that exposes a collection of interesting statistics about
	 * all the configured robots.
	 */
	class RobotInfoModel : public Glib::Object, public AbstractListModel {
		public:
			/**
			 * The column that shows the robot's name.
			 */
			Gtk::TreeModelColumn<Glib::ustring> name_column;

			/**
			 * The column that shows whether or not radio communication is
			 * established with this robot.
			 */
			Gtk::TreeModelColumn<bool> radio_column;

			/**
			 * The column that shows whether or not this robot is visible from
			 * the cameras.
			 */
			Gtk::TreeModelColumn<bool> visible_column;

			/**
			 * The column that shows the battery level in millivolts.
			 */
			Gtk::TreeModelColumn<unsigned int> battery_column;

			/**
			 * The column that shows the feedback interval in milliseconds.
			 */
			Gtk::TreeModelColumn<unsigned int> feedback_interval_column;

			/**
			 * The column that shows the run data interval in milliseconds.
			 */
			Gtk::TreeModelColumn<unsigned int> run_data_interval_column;

			/**
			 * Constructs a new RobotInfoModel.
			 *
			 * \param[in] bots the robots to display information about.
			 *
			 * \return the new model.
			 */
			static Glib::RefPtr<RobotInfoModel> create(const Config &conf, const std::vector<RefPtr<XBeeDriveBot> > &bots, const FriendlyTeam &friendly) {
				Glib::RefPtr<RobotInfoModel> mdl(new RobotInfoModel(conf, bots, friendly));
				return mdl;
			}

		private:
			const Config &conf;
			std::vector<RefPtr<XBeeDriveBot> > bots;
			std::vector<bool> visible;

			RobotInfoModel(const Config &conf, const std::vector<RefPtr<XBeeDriveBot> > &bots, const FriendlyTeam &friendly) : Glib::ObjectBase(typeid(RobotInfoModel)), conf(conf), bots(bots), visible(bots.size(), false) {
				alm_column_record.add(name_column);
				alm_column_record.add(radio_column);
				alm_column_record.add(visible_column);
				alm_column_record.add(battery_column);
				alm_column_record.add(feedback_interval_column);
				alm_column_record.add(run_data_interval_column);

				for (unsigned int i = 0; i < bots.size(); ++i) {
					if (bots[i]) {
						bots[i]->signal_feedback.connect(sigc::bind(sigc::mem_fun(this, &RobotInfoModel::alm_row_changed), i));
						bots[i]->signal_alive.connect(sigc::bind(sigc::mem_fun(this, &RobotInfoModel::alm_row_changed), i));
						bots[i]->signal_dead.connect(sigc::bind(sigc::mem_fun(this, &RobotInfoModel::alm_row_changed), i));
					}
				}

				friendly.signal_player_added.connect(sigc::mem_fun(this, &RobotInfoModel::on_player_added));
				friendly.signal_player_removed.connect(sigc::mem_fun(this, &RobotInfoModel::on_player_removed));
			}

			void on_player_added(unsigned int, RefPtr<Player> plr) {
				for (unsigned int i = 0; i < bots.size(); ++i) {
					if (bots[i] && bots[i]->address == plr->address()) {
						visible[i] = true;
						alm_row_changed(i);
					}
				}
			}

			void on_player_removed(unsigned int, RefPtr<Player> plr) {
				for (unsigned int i = 0; i < bots.size(); ++i) {
					if (bots[i] && bots[i]->address == plr->address()) {
						visible[i] = false;
						alm_row_changed(i);
					}
				}
			}

			unsigned int alm_rows() const {
				return bots.size();
			}

			void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const {
				if (col == static_cast<unsigned int>(name_column.index())) {
					Glib::Value<Glib::ustring> v;
					v.init(name_column.type());
					v.set(conf.robots()[row].name);
					value.init(name_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(radio_column.index())) {
					Glib::Value<bool> v;
					v.init(visible_column.type());
					v.set(bots[row] ? bots[row]->alive() : false);
					value.init(visible_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(visible_column.index())) {
					Glib::Value<bool> v;
					v.init(visible_column.type());
					v.set(visible[row]);
					value.init(visible_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(battery_column.index())) {
					Glib::Value<unsigned int> v;
					v.init(battery_column.type());
					v.set(bots[row] ? (bots[row]->alive() ? bots[row]->battery_voltage() : 0) : 0);
					value.init(battery_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(feedback_interval_column.index())) {
					Glib::Value<unsigned int> v;
					v.init(feedback_interval_column.type());
					if (bots[row] && bots[row]->alive()) {
						const timespec &ts(bots[row]->feedback_interval());
						unsigned int ms = timespec_to_millis(ts);
						v.set(ms);
					} else {
						v.set(0);
					}
					value.init(feedback_interval_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(run_data_interval_column.index())) {
					Glib::Value<unsigned int> v;
					v.init(run_data_interval_column.type());
					if (bots[row] && bots[row]->alive()) {
						const timespec &ts(bots[row]->run_data_interval());
						unsigned int ms = timespec_to_millis(ts);
						v.set(ms);
					} else {
						v.set(0);
					}
					value.init(run_data_interval_column.type());
					value = v;
				} else {
					std::abort();
				}
			}

			void alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &) {
			}
	};

	void battery_cell_data_func(Gtk::CellRenderer *r, const Gtk::TreeModel::iterator &iter, const Glib::RefPtr<RobotInfoModel> model) {
		unsigned int mv = iter->get_value(model->battery_column);
		Gtk::CellRendererProgress *rp = dynamic_cast<Gtk::CellRendererProgress *>(r);
		rp->property_value() = (clamp(mv, 12000U, 17000U) - 12000U) / 50U;
		if (mv) {
			rp->property_text() = Glib::ustring::compose("%1V", Glib::ustring::format(std::fixed, std::setprecision(2), mv / 1000.0));
		} else {
			rp->property_text() = "No Data";
		}
	}

	void interval_cell_data_func(Gtk::CellRenderer *r, const Gtk::TreeModel::iterator &iter, const Gtk::TreeModelColumn<unsigned int> &column) {
		unsigned int ms = iter->get_value(column);
		Gtk::CellRendererProgress *rp = dynamic_cast<Gtk::CellRendererProgress *>(r);
		rp->property_value() = clamp(ms, 0U, 1000U) / 10U;
		if (ms) {
			rp->property_text() = Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), ms / 1000.0));
		} else {
			rp->property_text() = "No Data";
		}
	}
}

AIWindow::AIWindow(AI &ai, bool show_vis) : ai(ai), strategy_controls(0), rc_controls(0), vis(ai.world->visualizer_view()) {
	set_title("AI");

	Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook);

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);

	Gtk::Frame *basic_frame = Gtk::manage(new Gtk::Frame("Basics"));
	Gtk::Table *basic_table = Gtk::manage(new Gtk::Table(5, 3));
	basic_table->attach(*Gtk::manage(new Gtk::Label("Play type override:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	playtype_override_chooser.append_text("<None>");
	playtype_override_chooser.set_active_text("<None>");
	for (unsigned int i = 0; i < PlayType::COUNT; ++i) {
		playtype_override_chooser.append_text(PlayType::DESCRIPTIONS_GENERIC[i]);
	}
	playtype_override_chooser.signal_changed().connect(sigc::mem_fun(this, &AIWindow::on_playtype_override_changed));
	basic_table->attach(playtype_override_chooser, 1, 3, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	basic_table->attach(*Gtk::manage(new Gtk::Label("Play type:")), 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	playtype_entry.set_editable(false);
	basic_table->attach(playtype_entry, 1, 3, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	basic_table->attach(*Gtk::manage(new Gtk::Label("Ball Filter:")), 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	ball_filter_chooser.append_text("<None>");
	for (BallFilter::map_type::const_iterator i = BallFilter::all().begin(), iend = BallFilter::all().end(); i != iend; ++i) {
		ball_filter_chooser.append_text(i->second->name);
	}
	BallFilter *ball_filter = ai.world->ball_filter();
	if (ball_filter) {
		ball_filter_chooser.set_active_text(ball_filter->name);
	} else {
		ball_filter_chooser.set_active_text("<None>");
	}
	ball_filter_chooser.signal_changed().connect(sigc::mem_fun(this, &AIWindow::on_ball_filter_changed));
	basic_table->attach(ball_filter_chooser, 1, 3, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	basic_table->attach(*Gtk::manage(new Gtk::Label("Defending:")), 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	end_entry.set_editable(false);
	basic_table->attach(end_entry, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	Gtk::Button *flip_ends_button = Gtk::manage(new Gtk::Button("X"));
	flip_ends_button->signal_clicked().connect(sigc::mem_fun(this, &AIWindow::on_flip_ends_clicked));
	basic_table->attach(*flip_ends_button, 2, 3, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	basic_table->attach(*Gtk::manage(new Gtk::Label("Refbox Colour:")), 0, 1, 4, 5, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	refbox_colour_entry.set_editable(false);
	basic_table->attach(refbox_colour_entry, 1, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	Gtk::Button *flip_refbox_colour_button = Gtk::manage(new Gtk::Button("X"));
	flip_refbox_colour_button->signal_clicked().connect(sigc::mem_fun(this, &AIWindow::on_flip_refbox_colour_clicked));
	basic_table->attach(*flip_refbox_colour_button, 2, 3, 4, 5, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	basic_frame->add(*basic_table);
	vbox->pack_start(*basic_frame, Gtk::PACK_SHRINK);

	Gtk::Frame *robots_frame = Gtk::manage(new Gtk::Frame("Robots"));
	const Glib::RefPtr<RobotInfoModel> robots_model(RobotInfoModel::create(ai.world->conf, ai.world->xbee_bots, ai.world->friendly));
	Gtk::TreeView *robots_tree = Gtk::manage(new Gtk::TreeView(robots_model));
	robots_tree->get_selection()->set_mode(Gtk::SELECTION_SINGLE);
	robots_tree->append_column("Name", robots_model->name_column);
	robots_tree->append_column("R", robots_model->radio_column);
	robots_tree->append_column("V", robots_model->visible_column);
	Gtk::CellRendererProgress *robots_battery_renderer = Gtk::manage(new Gtk::CellRendererProgress);
	int robots_battery_colnum = robots_tree->append_column("Battery", *robots_battery_renderer) - 1;
	Gtk::TreeViewColumn *robots_battery_column = robots_tree->get_column(robots_battery_colnum);
	robots_battery_column->set_cell_data_func(*robots_battery_renderer, sigc::bind(&battery_cell_data_func, robots_model));
	Gtk::CellRendererProgress *robots_feedback_interval_renderer = Gtk::manage(new Gtk::CellRendererProgress);
	int robots_feedback_interval_colnum = robots_tree->append_column("Feedback", *robots_feedback_interval_renderer) - 1;
	Gtk::TreeViewColumn *robots_feedback_interval_column = robots_tree->get_column(robots_feedback_interval_colnum);
	robots_feedback_interval_column->set_cell_data_func(*robots_feedback_interval_renderer, sigc::bind(&interval_cell_data_func, sigc::ref(robots_model->feedback_interval_column)));
	Gtk::CellRendererProgress *robots_run_data_interval_renderer = Gtk::manage(new Gtk::CellRendererProgress);
	int robots_run_data_interval_colnum = robots_tree->append_column("Run Data", *robots_run_data_interval_renderer) - 1;
	Gtk::TreeViewColumn *robots_run_data_interval_column = robots_tree->get_column(robots_run_data_interval_colnum);
	robots_run_data_interval_column->set_cell_data_func(*robots_run_data_interval_renderer, sigc::bind(&interval_cell_data_func, sigc::ref(robots_model->run_data_interval_column)));
	Gtk::ScrolledWindow *robots_scroller = Gtk::manage(new Gtk::ScrolledWindow);
	robots_scroller->add(*robots_tree);
	robots_scroller->set_shadow_type(Gtk::SHADOW_IN);
	robots_frame->add(*robots_scroller);
	vbox->pack_start(*robots_frame, Gtk::PACK_EXPAND_WIDGET);

	Gtk::Frame *strategy_frame = Gtk::manage(new Gtk::Frame("Strategy"));
	strategy_chooser.append_text("<Select Strategy>");
	for (StrategyFactory::map_type::const_iterator i = StrategyFactory::all().begin(), iend = StrategyFactory::all().end(); i != iend; ++i) {
		strategy_chooser.append_text(i->second->name);
	}
	const RefPtr<Strategy2> strategy(ai.get_strategy());
	if (strategy) {
		strategy_chooser.set_active_text(strategy->get_factory().name);
	} else {
		strategy_chooser.set_active_text("<Select Strategy>");
	}
	strategy_chooser.signal_changed().connect(sigc::mem_fun(this, &AIWindow::on_strategy_changed));
	strategy_vbox.pack_start(strategy_chooser, Gtk::PACK_SHRINK);
	put_strategy_controls();
	strategy_frame->add(strategy_vbox);
	vbox->pack_start(*strategy_frame, Gtk::PACK_SHRINK);

	Gtk::Frame *rc_frame = Gtk::manage(new Gtk::Frame("Robot Controller"));
	rc_chooser.append_text("<Select RC>");
	for (RobotControllerFactory::map_type::const_iterator i = RobotControllerFactory::all().begin(), iend = RobotControllerFactory::all().end(); i != iend; ++i) {
		rc_chooser.append_text(i->second->name);
	}
	RobotControllerFactory *robot_controller = ai.get_robot_controller_factory();
	if (robot_controller) {
		rc_chooser.set_active_text(robot_controller->name);
	} else {
		rc_chooser.set_active_text("<Select RC>");
	}
	rc_chooser.signal_changed().connect(sigc::mem_fun(this, &AIWindow::on_rc_changed));
	rc_vbox.pack_start(rc_chooser, Gtk::PACK_SHRINK);
	rc_frame->add(rc_vbox);
	vbox->pack_start(*rc_frame, Gtk::PACK_SHRINK);

	vis_button.set_label("Visualizer");
	vis_button.set_active(show_vis);
	vis_button.signal_toggled().connect(sigc::mem_fun(this, &AIWindow::on_vis_toggled));
	vbox->pack_start(vis_button, Gtk::PACK_SHRINK);

	vbox->pack_start(*Gtk::manage(new Annunciator), Gtk::PACK_EXPAND_WIDGET);

	notebook->append_page(*vbox, "Main");

	notebook->append_page(*Gtk::manage(new ParamPanel), "Params");

	add(*notebook);

	ai.world->signal_playtype_changed.connect(sigc::mem_fun(this, &AIWindow::on_playtype_changed));
	ai.world->signal_flipped_ends.connect(sigc::mem_fun(this, &AIWindow::on_flipped_ends));
	ai.world->signal_flipped_refbox_colour.connect(sigc::mem_fun(this, &AIWindow::on_flipped_refbox_colour));
	on_playtype_changed();
	on_flipped_ends();
	on_flipped_refbox_colour();

	show_all();

	vis.signal_overlay_changed.connect(sigc::mem_fun(this, &AIWindow::on_visualizer_overlay_changed));
	vis_window.set_title("AI Visualizer");
	vis_window.add(vis);
	vis_window.signal_delete_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::mem_fun(vis_button, &Gtk::ToggleButton::set_active), false), false)));
	if (show_vis) {
		vis_window.show_all();
	}
}

void AIWindow::on_playtype_override_changed() {
	const Glib::ustring &selected(playtype_override_chooser.get_active_text());
	for (unsigned int i = 0; i < PlayType::COUNT; ++i) {
		if (selected == PlayType::DESCRIPTIONS_GENERIC[i]) {
			ai.world->override_playtype(static_cast<PlayType::PlayType>(i));
			return;
		}
	}
	ai.world->clear_playtype_override();
}

void AIWindow::on_ball_filter_changed() {
	const Glib::ustring &name(ball_filter_chooser.get_active_text());
	BallFilter::map_type::const_iterator i = BallFilter::all().find(name.collate_key());
	if (i != BallFilter::all().end()) {
		ai.world->ball_filter(i->second);
	} else {
		ai.world->ball_filter(0);
	}
}

void AIWindow::on_flip_ends_clicked() {
	ai.world->flip_ends();
}

void AIWindow::on_flip_refbox_colour_clicked() {
	ai.world->flip_refbox_colour();
}

void AIWindow::on_strategy_changed() {
	const Glib::ustring &name(strategy_chooser.get_active_text());
	StrategyFactory::map_type::const_iterator i = StrategyFactory::all().find(name.collate_key());
	if (i != StrategyFactory::all().end()) {
		ai.set_strategy(i->second->create_strategy(ai.world));
	} else {
		ai.set_strategy(RefPtr<Strategy2>());
	}
	put_strategy_controls();
}

void AIWindow::on_rc_changed() {
	const Glib::ustring &name(rc_chooser.get_active_text());
	RobotControllerFactory::map_type::const_iterator i = RobotControllerFactory::all().find(name.collate_key());
	if (i != RobotControllerFactory::all().end()) {
		ai.set_robot_controller_factory(i->second);
	} else {
		ai.set_robot_controller_factory(0);
	}
}

void AIWindow::put_strategy_controls() {
	if (strategy_controls) {
		strategy_vbox.remove(*strategy_controls);
		strategy_controls = 0;
	}

	const RefPtr<Strategy2> strat(ai.get_strategy());
	if (strat) {
		strategy_controls = strat->get_ui_controls();
		if (!strategy_controls) {
			strategy_controls = Gtk::manage(new Gtk::Label("No controls."));
		}
	} else {
		strategy_controls = Gtk::manage(new Gtk::Label("No strategy selected."));
	}

	strategy_vbox.pack_start(*strategy_controls, Gtk::PACK_EXPAND_WIDGET);
	strategy_controls->show_all();
}

void AIWindow::on_playtype_changed() {
	playtype_entry.set_text(PlayType::DESCRIPTIONS_GENERIC[ai.world->playtype()]);
}

void AIWindow::on_vis_toggled() {
	if (vis_button.get_active()) {
		vis_window.show_all();
	} else {
		vis_window.hide_all();
	}
}

void AIWindow::on_flipped_ends() {
	end_entry.set_text(ai.world->east() ? "East" : "West");
}

void AIWindow::on_flipped_refbox_colour() {
	refbox_colour_entry.set_text(ai.world->refbox_yellow() ? "Yellow" : "Blue");
}

void AIWindow::on_visualizer_overlay_changed() {
	ai.set_overlay(vis.overlay());
}

