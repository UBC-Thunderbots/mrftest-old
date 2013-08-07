#ifndef AI_HL_STP_PLAY_SIMPLE_PLAY_H
#define AI_HL_STP_PLAY_SIMPLE_PLAY_H

#include "ai/hl/util.h"
#include "ai/hl/world.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/region.h"
#include "ai/hl/stp/world.h"

using AI::Common::PlayType;
using namespace AI::HL::STP::Predicates;
using namespace AI::HL::STP::Tactic;

using AI::HL::STP::TEAM_MAX_SIZE;
using AI::HL::STP::DIST_FROM_PENALTY_MARK;

#define BEGIN_PLAY(cls) \
	namespace { \
		class cls; \
		AI::HL::STP::Play::PlayFactoryImpl<cls> factory_instance(u8 ## # cls); \
		class cls : public AI::HL::STP::Play::Play { \
			public: \
				cls(AI::HL::W::World world) : AI::HL::STP::Play::Play(world) { \
				} \
 \
				AI::HL::STP::Play::PlayFactory &factory() const { \
					return factory_instance; \
				}

#define INVARIANT(expr) \
	bool invariant() const { \
		return expr; \
	}

#define APPLICABLE(expr) \
	bool applicable() const { \
		return expr; \
	}

#define DONE(expr) \
	bool done() const { \
		return expr; \
	}

#define FAIL(expr) \
	bool fail() const { \
		return expr; \
	}

#define BEGIN_ASSIGN() \
	void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[TEAM_MAX_SIZE-1]) {

#define END_ASSIGN() \
	}

#define END_PLAY() \
	}; \
	}

#endif

