#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/ssm/ssm.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	/**
	 * An implementation of the go to ball skill in the STP paper.
	 */
	class GoToBall : public Skill {
		private:
			void execute(World& world, Player::Ptr player, Param& param, Context& context) const {
				if (player->has_ball()) {
					const Skill* next = context.ssm()->initial();

					// Some SSMs start with GoToBall.
					// Need to prevent such a loop from happening.
					if (next == this) {
						player->move(player->position(), player->orientation(), 0, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
						return;
					}

					// umm not sure what to do next
					context.execute(next);
					return;
				}

				// TODO: check if enemy threatens the ball

				player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
			}
	};

	GoToBall go_to_ball_instance;
}

const Skill* AI::HL::STP::Skill::go_to_ball() {
	return &go_to_ball_instance;
}

