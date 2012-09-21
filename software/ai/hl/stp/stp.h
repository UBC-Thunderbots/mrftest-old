#ifndef AI_HL_STP_H
#define AI_HL_STP_H

#include "ai/hl/stp/world.h"
#include <cairomm/context.h>
#include <cairomm/refptr.h>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Run all evaluations.
			 */
			void tick_eval(World world);

			void draw_ui(World world, Cairo::RefPtr<Cairo::Context> ctx);

			Player::CPtr get_goalie();
		}
	}
}

#endif

