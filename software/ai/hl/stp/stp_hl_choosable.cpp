#include "ai/hl/hl.h"
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

	class STPHLChoosable : public PlayExecutor, public HighLevel {
		public:
			STPHLChoosable(World &world) : PlayExecutor(world) {
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

			void tick() {

				// override halt completely
				if (world.friendly_team().size() == 0 || world.playtype() == AI::Common::PlayType::HALT) {
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
				PlayExecutor::draw_overlay(ctx);
			}

		protected:
			Gtk::VBox vbox;
			Gtk::Button reset_button;
			Gtk::ComboBoxText combo;
	};

	HighLevel::Ptr STPHLChoosableFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new STPHLChoosable(world));
		return p;
	}
}

