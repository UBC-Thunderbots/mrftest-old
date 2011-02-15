#include "ai/hl/stp/skill/kick.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;
using AI::HL::STP::SSM::SkillStateMachine;

namespace {
	class Kick : public Skill {
		private:
			void execute(const World &world, Player::Ptr player, const SkillStateMachine *, Param &param, Context &context) const {
				// no ball lol; how did we get to this state.
				if (!player->has_ball()) {
					context.execute_after(go_to_ball());
					return;
				}

				// TODO: Might have to go after the ball regardless of has_ball check since camera may see ball inside player ><
				// player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);

				// stay at the same place, be oriented towards the ball so you can shoot (duh)
				player->move(player->position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);

				// TODO: check if can shoot
				if (player->chicker_ready_time() == 0) {
					player->kick(1.0);
					context.finish();
					return;
				}
			}
	};

	Kick kick_instance;
}

const Skill *AI::HL::STP::Skill::kick() {
	return &kick_instance;
}

