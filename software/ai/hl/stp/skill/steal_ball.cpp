#include "ai/hl/stp/skill/steal_ball.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;
using AI::HL::STP::SSM::SkillStateMachine;

namespace {
	class StealBall : public Skill {
		private:
			void execute(World& world, Player::Ptr player, const SkillStateMachine*, AI::HL::STP::Skill::Param& param, Context& context) const {

				const AI::HL::STP::Evaluation::BallThreat ball_threat = AI::HL::STP::Evaluation::evaluate_ball_threat(world);

				// can use normal grab ball mechanism
				if (!ball_threat.activate_steal) {
					context.execute_after(go_to_ball());
					return;
				}
#warning move to the front of the enemy robot to steal the ball
				//simply try to chase after the ball, but have a new movement type MOVE_STEAL?
				player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
			}
	};

	StealBall steal_ball_instance;
}

const Skill* AI::HL::STP::Skill::steal_ball() {
	return &steal_ball_instance;
}

