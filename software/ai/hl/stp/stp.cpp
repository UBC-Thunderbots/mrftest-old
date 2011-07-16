#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/defense.h"

using namespace AI::HL::STP;

namespace AI {
	namespace HL {
		namespace STP {
			extern Player::CPtr _goalie;
		}
	}
}

void AI::HL::STP::tick_eval(const World &world) {
	Evaluation::tick_offense(world);
	Evaluation::tick_defense(world);
	Evaluation::tick_ball(world);
}

void AI::HL::STP::draw_ui(const World &world, Cairo::RefPtr<Cairo::Context> ctx) {
	draw_shoot(world, ctx);
	draw_offense(world, ctx);
	draw_defense(world, ctx);
	draw_enemy_pass(world, ctx);
	draw_friendly_pass(world, ctx);
	draw_player_status(world, ctx);
	draw_baller(world, ctx);
}

Player::CPtr AI::HL::STP::get_goalie() {
	return _goalie;
}

