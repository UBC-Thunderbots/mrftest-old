#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "util/dprint.h"
#include <cassert>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/textview.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class STPHLChoosableFactory : public HighLevelFactory {
		public:
			STPHLChoosableFactory() : HighLevelFactory("STP-Selector") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	STPHLChoosableFactory factory_instance;

	const Glib::ustring CHOOSE_PLAY_TEXT = "<Choose Play>";

	class STPHLChoosable : public PlayExecutor, public HighLevel {
		public:
			Gtk::VBox vbox;
			Gtk::Button stop_button;
			Gtk::Button start_button;
			Gtk::TextView text_status;
			Gtk::ComboBoxText combo;

			STPHLChoosable(World &world) : PlayExecutor(world) {
				combo.append_text(CHOOSE_PLAY_TEXT);
				const Play::PlayFactory::Map &m = Play::PlayFactory::all();
				for (Play::PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					combo.append_text(i->second->name());
				}
				combo.set_active_text(CHOOSE_PLAY_TEXT);
				vbox.add(combo);
				vbox.add(start_button);
				vbox.add(stop_button);
				vbox.add(text_status);
				text_status.set_editable(false);
				start_button.set_label("start");
				stop_button.set_label("stop");

				start_button.signal_clicked().connect(sigc::bind(&STPHLChoosable::start, sigc::ref(*this)));
				stop_button.signal_clicked().connect(sigc::bind(&STPHLChoosable::stop, sigc::ref(*this)));
			}

			STPHLChoosableFactory &factory() const {
				return factory_instance;
			}

			void start() {
				LOG_INFO("start");

				// check if curr is valid
				if (curr_play.is()) {
					return;
				}
				// check what play is in use
				if (combo.get_active_text() == CHOOSE_PLAY_TEXT) {
					curr_play.reset();
					return;
				}
				calc_play();
			}

			void stop() {
				LOG_INFO("stop");

				if (curr_play.is()) {
					curr_play.reset();
				}
			}

			void calc_play() {
				curr_play.reset();
				for (auto i = plays.cbegin(), iend = plays.cend(); i != iend; ++i) {
					if ((*i)->factory().name() == combo.get_active_text()) {
						curr_play = *i;
					}
				}
				assert(curr_play.is());

				if (!curr_play->invariant() || curr_play->done() || curr_play->fail()) {
					LOG_WARN("play not valid");
					// curr_play.reset();
					// return;
				}

				// assign the players
				curr_role_step = 0;
				for (std::size_t j = 0; j < 5; ++j) {
					curr_roles[j].clear();
					// default to idle tactic
					curr_tactic[j] = Tactic::idle(world);
				}
				{
					std::vector<Tactic::Tactic::Ptr> goalie_role;
					std::vector<Tactic::Tactic::Ptr> normal_roles[4];
					curr_play->assign(goalie_role, normal_roles);
					swap(goalie_role, curr_roles[0]);
					for (std::size_t j = 1; j < 5; ++j) {
						swap(normal_roles[j - 1], curr_roles[j]);
					}
				}
				LOG_INFO("reassigned");
			}

			void tick() {
				tick_eval(world);

				// override halt completely
				if (world.friendly_team().size() == 0 || world.playtype() == AI::Common::PlayType::HALT) {
					curr_play.reset();
				}

				// check what play is in use
				if (combo.get_active_text() == CHOOSE_PLAY_TEXT) {
					curr_play.reset();
				}

				if (curr_play.is() && combo.get_active_text() != Glib::ustring(curr_play->factory().name())) {
					curr_play.reset();
				}

				/*
				   if (curr_play.is() && (!curr_play->invariant() || curr_play->done() || curr_play->fail())) {
				    LOG_INFO("play done/no longer valid");
				    curr_play.reset();
				   }
				 */

				std::ostringstream text;

				if (curr_play.is()) {
					text << "Running";
					execute_tactics();
				} else {
					text << "Stop";
				}

				if (curr_play.is()) {
					text << info();
				}

				text_status.get_buffer()->set_text(text.str());
			}

			Gtk::Widget *ui_controls() {
				return &vbox;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_ui(world, ctx);
			}
	};

	HighLevel::Ptr STPHLChoosableFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new STPHLChoosable(world));
		return p;
	}
}

