#include "ai/hl/strategy.h"
#include "uicomponents/param.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

#define TUNE_HALF

namespace {

	/**
	 * Manages the robots during a stoppage in place (that is, when the game is in PlayType::STOP).
	 */
	class MovementBenchmarkStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;
			void play();

			/**
			 * Creates a new MovementBenchmarkStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			MovementBenchmarkStrategy(World &world);
			~MovementBenchmarkStrategy();
	};

	/**
	 * A factory for constructing \ref MovementBenchmarkStrategy "StopStrategies".
	 */
	class MovementBenchmarkStrategyFactory : public StrategyFactory {
		public:
			MovementBenchmarkStrategyFactory();
			~MovementBenchmarkStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of MovementBenchmarkStrategyFactory.
	 */
	MovementBenchmarkStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
	};

	StrategyFactory &MovementBenchmarkStrategy::factory() const {
		return factory_instance;
	}

	void MovementBenchmarkStrategy::play() {
	}

	Strategy::Ptr MovementBenchmarkStrategy::create(World &world) {
		const Strategy::Ptr p(new MovementBenchmarkStrategy(world));
		return p;
	}

	MovementBenchmarkStrategy::MovementBenchmarkStrategy(World &world) : Strategy(world) {
	}

	MovementBenchmarkStrategy::~MovementBenchmarkStrategy() {
	}

	MovementBenchmarkStrategyFactory::MovementBenchmarkStrategyFactory() : StrategyFactory("Movement Benchmark", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	MovementBenchmarkStrategyFactory::~MovementBenchmarkStrategyFactory() {
	}

	Strategy::Ptr MovementBenchmarkStrategyFactory::create_strategy(World &world) const {
		return MovementBenchmarkStrategy::create(world);
	}
}

