#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/ui.h"
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
				draw_offense(world, ctx);
				draw_defense(world, ctx);
				//draw_velocity(ctx); // uncommand to display velocity
				return;
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
			
			void draw_velocity(Cairo::RefPtr<Cairo::Context> ctx) {
				const FriendlyTeam& friendly = world.friendly_team();
				ctx->set_line_width(1.0);
				for (std::size_t i = 0; i < friendly.size(); ++i ) {
					const Player::CPtr player = friendly.get(i);
					double vel_direction = atan( player->velocity().y / player->velocity().x );
					double vel_mag = sqrt( player->velocity().y * player->velocity().y + player->velocity().x * player->velocity().x );
					std::cout << vel_direction << "  " << vel_mag <<std::endl;
					ctx->set_source_rgba(0.0, 0.0, 0.0, 0.2);
					ctx->arc( player->position().x, player->position().y, vel_mag, vel_direction, vel_direction+1.0 );
					ctx->stroke();
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

