#include "ai/hl/hl.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/tactic/idle.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

#include <iostream>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class STPTestPassFactory : public HighLevelFactory {
		public:
			STPTestPassFactory() : HighLevelFactory("STP-TestPass do not call yet") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	STPTestPassFactory factory_instance;

	class STPTestPass : public PlayExecutor, public HighLevel {
		public:
		STPTestPass(World &world) : PlayExecutor(world), pos(0) {
				text_view.set_editable(false);
				const Play::PlayFactory::Map &m = Play::PlayFactory::all();
				assert(m.size() != 0);
				for (Play::PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					if(i->second->name() == "PassTest" ){
						plays.push_back(i->second->create(world));
					}
				}
				for (Play::PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					if(i->second->name() == "RecieveTest"){
						plays.push_back(i->second->create(world));
					}
				}
			}

			STPTestPassFactory &factory() const {
				return factory_instance;
			}

			void calc_play() {
				curr_play.reset();
				//need to set cur play here
				pos++;
				pos = pos % plays.size();
				curr_play = plays[pos];
				//need to set cur play above
				assert(curr_play.is());
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
				tick_ui();
			}

			Gtk::Widget *ui_controls() {
				return &text_view;
			}


			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				PlayExecutor::draw_overlay(ctx);
			}

		protected:

			int pos;
			/**
			* List of all the available plays
			*/
			std::vector<Play::Play::Ptr> plays;

			void tick_ui() {
				std::ostringstream text;
				if (curr_play.is()) {
					text << "play: " << curr_play->factory().name();
					text << "\n";
					text << "step: " << curr_role_step;
					text << "\n";
					for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
						text << curr_assignment[i]->pattern() << ": ";
						if (curr_tactic[i]->active()) {
							text << "*";
						} else {
							text << " ";
						}
						text << curr_tactic[i]->description();
						text << "\n";
					}
				} else {
					text << "No Play";
				}
				text_view.get_buffer()->set_text(text.str());
			}

		protected:
			Gtk::TextView text_view;
	};

	HighLevel::Ptr STPTestPassFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new STPTestPass(world));
		return p;
	}
}

