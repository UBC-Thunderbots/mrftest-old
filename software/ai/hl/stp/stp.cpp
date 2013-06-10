#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/tri_attack.h"

using namespace AI::HL::STP;

namespace AI {
	namespace HL {
		namespace STP {
			extern Player _goalie;
		}
	}
}

void AI::HL::STP::tick_eval(World world) {
	Evaluation::tick_ball(world);
	Evaluation::tick_offense(world);
	Evaluation::tick_defense(world);
	//Evaluation::tick_tri_attack(world);
}

void AI::HL::STP::draw_ui(World world, Cairo::RefPtr<Cairo::Context> ctx) {
	draw_shoot(world, ctx);
	draw_offense(world, ctx);
	draw_defense(world, ctx);
	draw_enemy_pass(world, ctx);
	draw_friendly_pass(world, ctx);
	draw_player_status(world, ctx);
	draw_baller(world, ctx);
}

Player AI::HL::STP::get_goalie() {
	return _goalie;
}

