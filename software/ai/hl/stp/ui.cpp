#include "ai/hl/stp/ui.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/predicates.h"
#include "geom/angle.h"
#include <cmath>

using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::grid_x;
using AI::HL::STP::Evaluation::grid_y;

namespace {
	BoolParam draw_ray(u8"draw ray", u8"Overlay/STP", false);
	BoolParam DRAW_FRIENDLY_PASS(u8"Draw friendly pass lines", u8"Overlay/STP", false);
	BoolParam DRAW_ENEMY_PASS(u8"Draw enemy pass lines", u8"Overlay/STP", false);
	BoolParam DRAW_SHOOT(u8"Draw shoot lines", u8"Overlay/STP", false);
	BoolParam DRAW_OFFENSE(u8"Draw offense circles", u8"Overlay/STP", false);
	BoolParam DRAW_DEFENSE(u8"Draw defense circles", u8"Overlay/STP", false);
}

void AI::HL::STP::draw_player_status(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	for (const Player player : world.friendly_team()) {
		if (!player.has_ball()) {
			continue;
		}
		ctx->set_source_rgba(1.0, 0.0, 0.0, 1.0);
		ctx->set_line_width(0.02);
		ctx->arc(player.position().x, player.position().y, Robot::MAX_RADIUS * 0.75, 0.0, 2 * M_PI);
		ctx->stroke();
	}
}

void AI::HL::STP::draw_friendly_pass(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	if(!DRAW_FRIENDLY_PASS) return;
	if (Predicates::our_ball(world)) {
		for (const Player i : world.friendly_team()) {
			if (Evaluation::passee_suitable(world, i)) {
				ctx->set_source_rgba(0.5, 1.0, 0.5, 0.5);
				ctx->set_line_width(0.01);
				ctx->move_to(i.position().x, i.position().y);
				ctx->line_to(world.ball().position().x, world.ball().position().y);
				ctx->stroke();
			}
		}
	}
}

void AI::HL::STP::draw_enemy_pass(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	if(!DRAW_ENEMY_PASS) return;
	auto threats = Evaluation::calc_enemy_threat(world);
	EnemyTeam enemy = world.enemy_team();

	for (std::size_t i = 0; i < enemy.size(); ++i) {
		Robot robot = threats[i].robot;
		Robot passee = threats[i].passee;
		if (threats[i].can_shoot_goal) {
			auto shot = Evaluation::calc_enemy_best_shot_goal(world, enemy[i]);
			ctx->set_source_rgba(1.0, 0.5, 0.5, 0.5);
			ctx->set_line_width(0.02);
			ctx->move_to(robot.position().x, robot.position().y);
			ctx->line_to(shot.first.x, shot.first.y);
			ctx->stroke();
		}
		if (threats[i].passes_goal > 2) {
			ctx->set_source_rgba(0.2, 0.2, 0.2, 0.5);
			ctx->arc(robot.position().x, robot.position().y, Robot::MAX_RADIUS + 0.01, 0.0, 2 * M_PI);
			ctx->fill();
			ctx->stroke();
		}
		if (threats[i].passes_goal <= 2 && passee) {
			ctx->set_source_rgba(1.0, 0.5, 0.5, 0.5);
			ctx->set_line_width(0.01);
			ctx->move_to(robot.position().x, robot.position().y);
			ctx->line_to(passee.position().x, passee.position().y);
			ctx->stroke();
		}
	}
}

void AI::HL::STP::draw_shoot(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	if(!DRAW_SHOOT) return;
	for (const Player player : world.friendly_team()) {
		Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);

		// draw yellow circle
		if (!shoot_data.can_shoot) {
			continue;
		}

		ctx->set_source_rgba(0.0, 1.0, 1.0, 0.4);
		if (Evaluation::possess_ball(world, player)) {
			ctx->set_line_width(Robot::MAX_RADIUS);
		} else {
			ctx->set_line_width(0.02);
		}
		ctx->move_to(player.position().x, player.position().y);
		ctx->line_to(shoot_data.target.x, shoot_data.target.y);
		ctx->stroke();
	}

	Player baller = Evaluation::calc_friendly_baller();
	if (baller) {
		auto shot = Evaluation::best_shoot_ray(world, baller);

		if (shot.first) {
			const Angle angle = shot.second;
			const Point p1 = baller.position();
			const Point p2 = p1 + 5 * Point::of_angle(angle);
			if (Evaluation::can_shoot_ray(world, baller, angle)) {
				ctx->set_source_rgba(0.0, 1.0, 1.0, 0.4);
				ctx->set_line_width(Robot::MAX_RADIUS);
				ctx->move_to(p1.x, p1.y);
				ctx->line_to(p2.x, p2.y);
				ctx->stroke();
			}
		}
	}

	if (draw_ray) {
		for (const Player player : world.friendly_team()) {
			if (!Evaluation::possess_ball(world, player)) {
				continue;
			}

			// draw rays for ray shooting
			const Angle angle_span = 2 * Evaluation::max_pass_ray_angle;
			const Angle angle_step = angle_span / Evaluation::ray_intervals;
			const Angle angle_min = player.orientation() - angle_span / 2;

			for (int i = 0; i < Evaluation::ray_intervals; ++i) {
				const Angle angle = angle_min + angle_step * i;

				const Point p1 = player.position();
				const Point p2 = p1 + 3 * Point::of_angle(angle);
				if (Evaluation::can_shoot_ray(world, player, angle)) {
					ctx->set_source_rgba(1.0, 0.0, 1.0, 0.4);
					ctx->set_line_width(0.02);
					ctx->move_to(p1.x, p1.y);
					ctx->line_to(p2.x, p2.y);
					ctx->stroke();
				}
			}
		}
	}
}

void AI::HL::STP::draw_offense(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	if(!DRAW_OFFENSE) return;
	// draw blue circles for offense
	{
		// divide up into grids
		const double x1 = -world.field().length() / 2;
		const double x2 = world.field().length() / 2;
		const double y1 = -world.field().width() / 2;
		const double y2 = world.field().width() / 2;

		const double dx = (x2 - x1) / (grid_x + 1) / 2;
		const double dy = (y2 - y1) / (grid_y + 1) / 2;

		for (unsigned int i = 1; i <= 2 * static_cast<unsigned int>(grid_y) + 1; i += 2) {
			for (unsigned int j = (i / 2 + 1) % 2 + 1; j <= 2 * static_cast<unsigned int>(grid_x) + 1; j += 2) {
				const double x = x1 + dx * j;
				const double y = y1 + dy * i;
				//const Point pos = Point(x, y);

				const double score = Evaluation::offense_score(i, j);

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

				const double radius = std::min(std::sqrt(score) * 0.01, 0.1);

				ctx->set_source_rgba(0.5, 0.5, 1.0, 0.2);
				ctx->arc(x, y, radius, 0.0, 2 * M_PI);
				ctx->fill();
				ctx->stroke();
			}
		}
	}

	// draw green circles for best offense
	{
		std::array<Point, 2> positions = Evaluation::offense_positions();
		ctx->set_source_rgba(0.6, 1.0, 0.6, 0.8);
		ctx->arc(positions[0].x, positions[0].y, 0.1, 0.0, 2 * M_PI);
		ctx->fill();
		ctx->stroke();

		ctx->set_source_rgba(0.6, 1.0, 0.6, 0.7);
		ctx->arc(positions[1].x, positions[1].y, 0.1, 0.0, 2 * M_PI);
		ctx->fill();
		ctx->stroke();
	}
}

void AI::HL::STP::draw_defense(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	if(!DRAW_DEFENSE) return;
	const Field &field = world.field();

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

	// draw own goal?
	/*
	   ctx->set_line_width(0.01);
	   ctx->set_source_rgba(1.0, 0.5, 0.5, 1.0);
	   ctx->move_to(goal1.x, goal1.y);
	   ctx->line_to(goal2.x, goal2.y);
	   ctx->stroke();
	 */
}

void AI::HL::STP::draw_velocity(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	ctx->set_line_width(1.0);
	for (const Player player : world.friendly_team()) {
		double vel_direction = atan(player.velocity().y / player.velocity().x);
		double vel_mag = player.velocity().len();
		// std::cout << vel_direction << "  " << vel_mag <<std::endl;
		ctx->set_source_rgba(0.0, 0.0, 0.0, 0.2);
		ctx->arc(player.position().x, player.position().y, vel_mag, vel_direction, vel_direction + 1.0);
		ctx->stroke();
	}
}

void AI::HL::STP::draw_baller(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	Player baller = Evaluation::calc_friendly_baller();
	if (baller && !Evaluation::possess_ball(world, baller)) {
		Point dest = Evaluation::calc_fastest_grab_ball_dest(world, baller);
		// black line
		ctx->set_source_rgba(0.0, 0.0, 0.0, 0.2);
		ctx->set_line_width(0.01);
		ctx->move_to(baller.position().x, baller.position().y);
		ctx->line_to(dest.x, dest.y);
		ctx->stroke();
	}
	Robot robot = Evaluation::calc_enemy_baller(world);
	if (robot && !Evaluation::possess_ball(world, robot)) {
		// black line
		Point dest;
		AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), robot.position(), dest);
		ctx->set_source_rgba(0.0, 0.0, 0.0, 0.2);
		ctx->set_line_width(0.01);
		ctx->move_to(robot.position().x, robot.position().y);
		ctx->line_to(dest.x, dest.y);
		ctx->stroke();
	}
}

