#ifndef AI_HL_STP_EVALUATE_SHOOT_H
#define AI_HL_STP_EVALUATE_SHOOT_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluate {
				struct ShootStats {
					Point target;
					double angle;
					bool can_shoot;
					bool ball_on_front;
					bool ball_visible;
				};

				//class EvaluateShoot : public Cacheable<ShootStats, CacheableNonKeyArgs<AI::HL::W::World &>, CacheableKeyArgs<AI::HL::W::Player::Ptr> > {
					//ShootStats compute(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) const;
				//};

				const ShootStats shoot_stats(AI::HL::W::World &world, AI::HL::W::Player::Ptr player);
			}
		}
	}
}

#endif

