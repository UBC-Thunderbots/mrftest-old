#ifndef AI_HL_STP_EVALUATION_SHOOT_H
#define AI_HL_STP_EVALUATION_SHOOT_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				struct ShootData {
					Point target;
					double angle;
					bool can_shoot;
					bool ball_on_front;
					bool ball_visible;
				};

				/*
				class EvaluateShoot : public Cacheable<ShootData, CacheableNonKeyArgs<AI::HL::W::World &>, CacheableKeyArgs<AI::HL::W::Player::Ptr> > {
					protected:
						ShootData compute(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) const;
				};

				extern EvaluateShoot evaluate_shoot;
				*/

				ShootData evaluate_shoot(const AI::HL::W::World &world, AI::HL::W::Player::CPtr player);
			}
		}
	}
}

#endif

