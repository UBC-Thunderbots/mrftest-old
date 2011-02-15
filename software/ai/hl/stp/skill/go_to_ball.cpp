#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/ssm/ssm.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;
using AI::HL::STP::SSM::SkillStateMachine;

namespace {
	/**
	 * An implementation of the go to ball skill in the STP paper.
	 */
	class GoToBall : public Skill {
		private:
			void execute(const World& world, Player::Ptr player, const SkillStateMachine* ssm, Param& param, Context& context) const {
				if (player->has_ball()) {
					const Skill* next = ssm->initial();

					// Some SSMs start with GoToBall.
					// Need to prevent such a loop from happening.
					if (next == this) {
						player->move(player->position(), player->orientation(), 0, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
						return;
					}

					// umm not sure what to do next
					context.execute_after(next);
					return;
				}

				// TODO: check if enemy threatens the ball
				
				const AI::HL::STP::Evaluation::BallThreat ball_threat = AI::HL::STP::Evaluation::evaluate_ball_threat(world);

				// can use normal grab ball mechanism
				if (ball_threat.activate_steal) {
					#warning move to the front of the enemy robot to steal the ball
					//simply try to chase after the ball, but have a new movement type MOVE_STEAL?
					player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
				}

				player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
			}
	};

	GoToBall go_to_ball_instance;
}

const Skill* AI::HL::STP::Skill::go_to_ball() {
	return &go_to_ball_instance;
}

