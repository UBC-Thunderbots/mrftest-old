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
	const Glib::ustring CHOOSE_PLAY_TEXT = u8"<Choose Play>";

	class STPHLChoosable final : public PlayExecutor, public HighLevel {
		public:
			Gtk::VBox vbox;
			Gtk::Button stop_button;
			Gtk::Button start_button;
			Gtk::TextView text_status;
			Gtk::ComboBoxText combo;

			explicit STPHLChoosable(World world) : PlayExecutor(world) {
				combo.append(CHOOSE_PLAY_TEXT);
				for (const auto &i : Play::PlayFactory::all()) {
					combo.append(i.second->name());
				}
				combo.set_active_text(CHOOSE_PLAY_TEXT);
				vbox.add(combo);
				vbox.add(start_button);
				vbox.add(stop_button);
				vbox.add(text_status);
				text_status.set_editable(false);
				start_button.set_label(u8"start");
				stop_button.set_label(u8"stop");

				start_button.signal_clicked().connect(sigc::bind(&STPHLChoosable::start, sigc::ref(*this)));
				stop_button.signal_clicked().connect(sigc::bind(&STPHLChoosable::stop, sigc::ref(*this)));
			}

			HighLevelFactory &factory() const;

			void start() {
				LOG_INFO(u8"start");

				// check if curr is valid
				if (curr_play) {
					return;
				}
				// check what play is in use
				if (combo.get_active_text() == CHOOSE_PLAY_TEXT) {
					curr_play = nullptr;
					return;
				}
				calc_play();
			}

			void stop() {
				LOG_INFO(u8"stop");

				curr_play = nullptr;
			}

			void calc_play() override {
				curr_play = nullptr;
				for (const auto &i : plays) {
					if (i->factory().name() == combo.get_active_text()) {
						curr_play = i.get();
					}
				}
				assert(curr_play);

				if (!curr_play->invariant() || curr_play->done() || curr_play->fail()) {
					// only warn but still execute
					LOG_WARN(u8"play not valid");
				}

				// assign the players
				curr_role_step = 0;
				for (std::size_t j = 0; j < TEAM_MAX_SIZE; ++j) {
					// default to idle tactic
					curr_tactic[j] = idle_tactics[j].get();
					curr_roles[j].clear();
				}
				{
					std::vector<Tactic::Tactic::Ptr> goalie_role;
					std::vector<Tactic::Tactic::Ptr> normal_roles[TEAM_MAX_SIZE-1];
					curr_play->assign(goalie_role, normal_roles);
					swap(goalie_role, curr_roles[0]);
					for (std::size_t j = 1; j < TEAM_MAX_SIZE; ++j) {
						swap(normal_roles[j - 1], curr_roles[j]);
					}
				}
				LOG_INFO(u8"reassigned");
			}

			void tick() override {
				tick_eval(world);
				enable_players();

				// override halt completely
				if (world.friendly_team().size() == 0 || world.playtype() == AI::Common::PlayType::HALT) {
					curr_play = nullptr;
				}

				// check what play is in use
				if (combo.get_active_text() == CHOOSE_PLAY_TEXT) {
					curr_play = nullptr;
				}

				if (curr_play && combo.get_active_text() != Glib::ustring(curr_play->factory().name())) {
					curr_play = nullptr;
				}

				/*
				   if (curr_play && (!curr_play->invariant() || curr_play->done() || curr_play->fail())) {
				    LOG_INFO(u8"play done/no longer valid");
				    curr_play = nullptr;
				   }
				 */

				Glib::ustring text;

				if (curr_play) {
					text = u8"Running";
					execute_tactics();
				} else {
					text = u8"Stop";
				}

				if (curr_play) {
					text += info();
				}

				text_status.get_buffer()->set_text(text);
			}

			Gtk::Widget *ui_controls() override {
				return &vbox;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override {
				draw_ui(world, ctx);
				if (world.playtype() == AI::Common::PlayType::STOP) {
					ctx->set_source_rgb(1.0, 0.5, 0.5);
					ctx->arc(world.ball().position().x, world.ball().position().y, 0.5, 0.0, 2 * M_PI);
					ctx->stroke();
				}
				if(!curr_play)
					return;
				curr_play->draw_overlay(ctx);
				for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
					if (!curr_assignment[i]) {
						continue;
					}
					const std::vector<Tactic::Tactic::Ptr> &role = curr_roles[i];
					for (std::size_t t = 0; t < role.size(); ++t) {
						role[t]->draw_overlay(ctx);
					}
				}
			}
	};
}

HIGH_LEVEL_REGISTER(STPHLChoosable)

