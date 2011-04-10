#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "util/dprint.h"

#include <iostream>
#include <math.h>
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
			
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_offense(ctx);
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

			void draw_offense(Cairo::RefPtr<Cairo::Context> ctx) {

				// draw yellow circles for shooting
				const FriendlyTeam& friendly = world.friendly_team();
				for (std::size_t i = 0; i < friendly.size(); ++i) {
					const Player::CPtr player = friendly.get(i);
					std::pair<Point, double> best_shot = AI::HL::Util::calc_best_shot(world, player);
					if (best_shot.second < AI::HL::Util::shoot_accuracy * M_PI / 180) {
						continue;
					}

					const double radius = best_shot.second * 1.0;

					// draw yellow circle
					ctx->set_source_rgba(1.0, 1.0, 0.5, 0.2);
					ctx->arc(player->position().x, player->position().y, radius, 0.0, 2 * M_PI);
					ctx->fill();
					ctx->stroke();

					// draw line
					ctx->set_source_rgba(1.0, 1.0, 0.5, 0.2);
					ctx->set_line_width(0.01);
					ctx->move_to(player->position().x, player->position().y);
					ctx->line_to(best_shot.first.x, best_shot.first.y);
					ctx->stroke();
				}

				// draw blue circles for offense

				const int GRID_X = 20;
				const int GRID_Y = 20;

				// divide up into grids
				const double x1 = -world.field().length() / 2;
				const double x2 = world.field().length() / 2;
				const double y1 = -world.field().width() / 2;
				const double y2 = world.field().width() / 2;

				const double dx = (x2 - x1) / (GRID_X + 1);
				const double dy = (y2 - y1) / (GRID_Y + 1);

				for (int i = 0; i < GRID_X; ++i) {
					for (int j = 0; j < GRID_Y; ++j) {
						const double x = x1 + dx * (i + 1);
						const double y = y1 + dy * (j + 1);
						const Point pos = Point(x, y);

						const double score = Evaluation::offense_score(world, pos);

						/*
						   {
						   std::ostringstream text;
						   text << score << std::endl;
						   LOG_INFO(text.str());
						   }
						   */

						if (score < 0) {
							continue;
						}

						const double radius = score * 0.01;

						ctx->set_source_rgba(0.5, 0.5, 1.0, 0.2);
						ctx->arc(x, y, radius, 0.0, 2 * M_PI);
						ctx->fill();
						ctx->stroke();
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

