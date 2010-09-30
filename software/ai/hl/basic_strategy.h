#ifndef AI_HL_BASIC_STRATEGY_H
#define AI_HL_BASIC_STRATEGY_H

#include "ai/hl/strategy.h"
#include "ai/hl/defender.h"

namespace AI {
	namespace HL {
		/**
		 * A full implementation of a strategy that handles normal play.
		 */
		class BasicStrategy : public AI::HL::Strategy {
			public:
				StrategyFactory &factory() const;
				static Strategy::Ptr create(AI::HL::W::World &world);

				/**
				 * The reason we have this class.
				 */
				void play();

			private:
				BasicStrategy(AI::HL::W::World &world);
				~BasicStrategy();
				void on_play_type_changed();

				Defender defender;
		};
	}
}

#endif

