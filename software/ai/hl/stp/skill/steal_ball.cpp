#include "ai/hl/stp/skill/steal_ball.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/evaluate/ball_threat.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	class StealBall : public Skill {
		private:
			const Skill* execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, AI::HL::STP::Skill::Param& param) const {

				const AI::HL::STP::Evaluate::BallThreat ball_threat = AI::HL::STP::Evaluate::evaluate_ball_threat(world);

				// can use normal grab ball mechanism
				if (!ball_threat.activate_steal) {
					return go_to_ball()->execute(world, player, ssm, param);
				}

#warning move to the front of the enemy robot to steal the ball

				return this;
			}
	};

	StealBall steal_ball_instance;
}

const Skill* AI::HL::STP::Skill::steal_ball() {
	return &steal_ball_instance;
}

