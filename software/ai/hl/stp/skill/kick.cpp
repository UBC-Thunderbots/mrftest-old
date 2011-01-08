#include "ai/hl/stp/skill/kick.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	Kick kick;
}

const Kick* Kick::instance() {
	return &kick;
}

const Skill* Kick::execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const {
	// no ball lol; how did we get to this state.
	if (!player->has_ball()) {
		return GoToBall::instance()->execute(world, player, ssm, param);
	}

	// stay at the same place
	player->move(player->position(), player->orientation(), 0, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);

	// TODO: check if can shoot
	if (player->chicker_ready_time() == 0) {
		player->kick(1.0);
	}
	return this;
}

