#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "util/dprint.h"
#include <cmath>
#include <gtkmm.h>
#include <iostream>
#include <sstream>

using AI::HL::HighLevelFactory;
using AI::HL::HighLevel;
using namespace AI::HL::STP;

namespace {
	class STPHLFactory : public HighLevelFactory {
		public:
			STPHLFactory() : HighLevelFactory("STP") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	STPHLFactory factory_instance;

	class STPHL : public PlayExecutor, public HighLevel {
		public:
			STPHL(World &world) : PlayExecutor(world) {
				text_view.set_editable(false);
			}

			STPHLFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				PlayExecutor::tick();
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

			Gtk::Widget *ui_controls() {
				return &text_view;
			}
			
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				PlayExecutor::draw_overlay(ctx);
			}
		
		protected:
			Gtk::TextView text_view;
	};

	HighLevel::Ptr STPHLFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new STPHL(world));
		return p;
	}
}

