#include "ai/hl/stp/skill/kick.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	class Kick : public Skill {
		private:
			void execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, Param& param, Context& context) const {
				// no ball lol; how did we get to this state.
				if (!player->has_ball()) {
					context.execute(go_to_ball());
					return;
				}

				// stay at the same place
				player->move(player->position(), player->orientation(), param.move_flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);

				// TODO: check if can shoot
				if (player->chicker_ready_time() == 0) {
					player->kick(1.0);
					context.transition(finish());
					return;
				}
			}
	};

	Kick kick_instance;
}

const Skill* AI::HL::STP::Skill::kick() {
	return &kick_instance;
}

