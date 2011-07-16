#ifndef AI_HL_STP_H
#define AI_HL_STP_H

#include "ai/hl/stp/world.h"

#include <gtkmm.h>
#include <string>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Run all evaluations.
			 */
			void tick_eval(const World &world);

			void draw_ui(const World &world, Cairo::RefPtr<Cairo::Context> ctx);

			Player::CPtr get_goalie();
		}
	}
}

#endif

