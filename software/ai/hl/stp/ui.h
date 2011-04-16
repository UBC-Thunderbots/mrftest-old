#ifndef AI_HL_STP_UI_H
#define AI_HL_STP_UI_H

#include <gtkmm.h>
#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * draw blue circles to indicate good offensive position.
			 * draw yellow halo around robots to indicate how well they can shoot the enemy goal.
			 */
			void draw_offense(const World& world, Cairo::RefPtr<Cairo::Context> ctx);

			/**
			 * draw lines from the ball to the sides of our goal post
			 * TODO: draw lines from the enemy
			 */
			void draw_defense(const World& world, Cairo::RefPtr<Cairo::Context> ctx);
		}
	}
}

#endif
