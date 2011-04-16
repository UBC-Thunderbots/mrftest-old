#include "ai/hl/hl.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/tactic/idle.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

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

	class STPHLChoosable : public HighLevel {
		public:
			STPHLChoosable(World &world) : world(world) {
				combo.append_text(CHOOSE_PLAY_TEXT);
				const Play::PlayFactory::Map &m = Play::PlayFactory::all();
				for (Play::PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					combo.append_text(i->second->name());
				}
				combo.set_active_text(CHOOSE_PLAY_TEXT);
				vbox.add(combo);
				// TODO: add reset
				//vbox.add(reset_button);
				reset_button.set_label("reset");
			}

			STPHLChoosableFactory &factory() const {
				return factory_instance;
			}

		private:
			World& world;
			Gtk::VBox vbox;
			Gtk::Button reset_button;
			Gtk::ComboBoxText combo;

			Play::Play::Ptr curr_play;
			unsigned int curr_role_step;
			std::vector<Tactic::Tactic::Ptr> curr_roles[5];
			Tactic::Tactic::Ptr curr_tactic[5];
			Tactic::Tactic::Ptr curr_active;
			AI::HL::W::Player::Ptr curr_assignment[5];
			std::vector<Play::Play::Ptr> plays;

			void calc_play() {
				curr_play.reset();
				const Play::PlayFactory::Map &m = Play::PlayFactory::all();
				for (Play::PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					if (Glib::ustring(i->second->name()) != combo.get_active_text()) {
						continue;
					}
					curr_play = i->second->create(world);
				}
				assert(curr_play.is());
				//auto itr = m.find(combo.get_active_text().c_str());
				//assert(itr != m.end());
				//curr_play = m[combo.get_active_text()]->create(world);
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

			void role_assignment() {
				// this must be reset every tick
				curr_active.reset();

				for (std::size_t i = 0; i < 5; ++i) {
					if (curr_role_step < curr_roles[i].size()) {
						curr_tactic[i] = curr_roles[i][curr_role_step];
					} else {
						// if there are no more tactics, use the previous one
						// BUT active tactic cannot be reused!
						assert(!curr_tactic[i]->active());
					}

					if (curr_tactic[i]->active()) {
						// we cannot have more than 1 active tactic.
						assert(!curr_active.is());
						curr_active = curr_tactic[i];
					}
				}

				// we cannot have less than 1 active tactic.
				assert(curr_active.is());

				std::fill(curr_assignment, curr_assignment + 5, AI::HL::W::Player::Ptr());

				assert(curr_tactic[0].is());
				curr_tactic[0]->set_player(world.friendly_team().get(0));
				curr_assignment[0] = world.friendly_team().get(0);

				// pool of available people
				std::set<Player::Ptr> players;
				for (std::size_t i = 1; i < world.friendly_team().size(); ++i) {
					players.insert(world.friendly_team().get(i));
				}

				bool active_assigned = (curr_tactic[0]->active());
				for (std::size_t i = 1; i < 5 && players.size() > 0; ++i) {
					curr_assignment[i] = curr_tactic[i]->select(players);
					// assignment cannot be empty
					assert(curr_assignment[i].is());
					assert(players.find(curr_assignment[i]) != players.end());
					players.erase(curr_assignment[i]);
					curr_tactic[i]->set_player(curr_assignment[i]);
					active_assigned = active_assigned || curr_tactic[i]->active();
				}

				// can't assign active tactic to anyone
				if (!active_assigned) {
					LOG_ERROR("Active tactic not assigned");
					curr_play.reset();
					return;
				}
			}

			void execute_tactics() {
				std::size_t max_role_step = 0;
				for (std::size_t i = 0; i < 5; ++i) {
					max_role_step = std::max(max_role_step, curr_roles[i].size());
				}

				while (true) {
					role_assignment();

					// if role assignment failed
					if (!curr_play.is()) {
						return;
					}

					// it is possible to skip steps
					if (curr_active->done()) {
						++curr_role_step;

						// when the play runs out of tactics, they are done!
						if (curr_role_step >= max_role_step) {
							LOG_INFO("Play done");
							curr_play.reset();
							return;
						}

						continue;
					}

					break;
				}

				// execute!
				for (std::size_t i = 0; i < 5; ++i) {
					if (!curr_assignment[i].is()) {
						continue;
					}
					curr_tactic[i]->execute();
				}
			}

			void tick() {

				// override halt completely
				if (world.friendly_team().size() == 0 || world.playtype() == PlayType::HALT) {
					curr_play.reset();
					return;
				}
	
				// check what play is in use
				if (combo.get_active_text() == CHOOSE_PLAY_TEXT) {
					curr_play.reset();
					return;
				}

				if (curr_play.is() && combo.get_active_text() != Glib::ustring(curr_play->factory().name())) {
					curr_play.reset();
				}

				if (curr_play.is() && (!curr_play->invariant() || curr_play->done() || curr_play->fail())) {
					curr_play.reset();
				}

				// check if curr is valid
				if (!curr_play.is()) {
					calc_play();
					if (!curr_play.is()) {
						LOG_WARN("calc play failed");
						return;
					}
				}

				execute_tactics();
			}

			Gtk::Widget *ui_controls() {
				return &vbox;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_offense(world, ctx);
				draw_defense(world, ctx);

				if (world.playtype() == PlayType::STOP) {
					ctx->set_source_rgb(1.0, 0.5, 0.5);
					ctx->arc(world.ball().position().x, world.ball().position().y, 0.5, 0.0, 2 * M_PI);
					ctx->stroke();
				}
				if (!curr_play.is()) {
					return;
				}
				for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
					const auto& role = curr_roles[i];
					for (std::size_t t = 0; t < role.size(); ++t) {
						role[t]->draw_overlay(ctx);
					}
				}
			}
	};

	HighLevel::Ptr STPHLChoosableFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new STPHLChoosable(world));
		return p;
	}
}

