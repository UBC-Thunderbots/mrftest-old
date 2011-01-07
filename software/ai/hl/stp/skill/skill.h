#ifndef AI_HL_STP_SKILL_SKILL_H
#define AI_HL_STP_SKILL_SKILL_H

#include "ai/hl/world.h"
#include "util/byref.h"
#include "util/registerable.h"
#include "util/fiber.h"

#include <ctime>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {

				/**
				 * A skill is the bottomost layer in the STP paradigm.
				 * It contains the basic component of movement.
				 */
				class Skill : public ByRef {
					public:
						typedef RefPtr<Skill> Ptr;

						/**
						 * The content of the skill shall go here.
						 */
						virtual void run() = 0;

					protected:
						Skill(AI::HL::W::World& w, AI::HL::W::Player::Ptr p);

						AI::HL::W::World& world;
						AI::HL::W::Player::Ptr player;
				};
			}

		}
	}
}

#endif

