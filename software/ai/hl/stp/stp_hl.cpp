#include "ai/hl/hl.h"
#include "ai/hl/stp/play_executor.h"

#include <sstream>
#include <gtkmm.h>

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
						text << world.friendly_team().get(i)->pattern() << ": ";
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

			void draw_overlay(Cairo::RefPtr<Cairo::Context> context) {
				if (!curr_play.is()) {
					return;
				}
				for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
					const auto& role = curr_roles[i];
					for (std::size_t t = 0; t < role.size(); ++t) {
						role[t]->draw_overlay(context);
					}
				}
			}

		protected:
			Gtk::TextView text_view;
	};

	HighLevel::Ptr STPHLFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new STPHL(world));
		return p;
	}
}

