#ifndef AI_HL_STP_UI_H
#define AI_HL_STP_UI_H

#include "ai/hl/stp/world.h"
#include <cairomm/context.h>
#include <cairomm/refptr.h>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Draw yellow circles on robots that can shoot the goal.
			 */
			void draw_shoot(const World &world, Cairo::RefPtr<Cairo::Context> ctx);

			void draw_enemy_pass(const World &world, Cairo::RefPtr<Cairo::Context> ctx);

			void draw_friendly_pass(const World &world, Cairo::RefPtr<Cairo::Context> ctx);

			/**
			 * draw blue circles to indicate good offensive position.
			 * draw yellow halo around robots to indicate how well they can shoot the enemy goal.
			 */
			void draw_offense(const World &world, Cairo::RefPtr<Cairo::Context> ctx);

			/**
			 * draw lines from the ball to the sides of our goal post
			 * TODO: draw lines from the enemy
			 */
			void draw_defense(const World &world, Cairo::RefPtr<Cairo::Context> ctx);

			void draw_velocity(const World &world, Cairo::RefPtr<Cairo::Context> ctx);

			void draw_player_status(const World &world, Cairo::RefPtr<Cairo::Context> ctx);

			void draw_baller(const World &world, Cairo::RefPtr<Cairo::Context> ctx);
		}
	}
}

#endif

