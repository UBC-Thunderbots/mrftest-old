#include "ai/hl/stp/ui.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/offense.h"

using namespace AI::HL::STP;

void AI::HL::STP::draw_offense(const World& world, Cairo::RefPtr<Cairo::Context> ctx) {

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

void AI::HL::STP::draw_defense(const World& world, Cairo::RefPtr<Cairo::Context> ctx) {
	const Field& field = world.field();

	const Point goal1 = Point(-field.length() / 2, field.goal_width() / 2);
	const Point goal2 = Point(-field.length() / 2, -field.goal_width() / 2);

	ctx->set_line_width(0.01);
	ctx->set_source_rgba(0.5, 1.0, 0.5, 0.5);

	ctx->move_to(world.ball().position().x, world.ball().position().y);
	ctx->line_to(goal1.x, goal1.y);
	ctx->stroke();

	ctx->move_to(world.ball().position().x, world.ball().position().y);
	ctx->line_to(goal2.x, goal2.y);
	ctx->stroke();
}

