#include "ai/hl/strategy.h"
#include "ai/hl/stp/play_executor.h"

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using AI::HL::STP::PlayExecutor;
using namespace AI::HL::W;

namespace {
	/**
	 * STP based strategy
	 * STP: Skills, tactics and plays for multi-robot control in adversarial environments
	 */
	class STPStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			static Strategy::Ptr create(World &world);

		protected:
			PlayExecutor play_executor;

			STPStrategy(World &world);

			~STPStrategy();

			void halt();
			void stop();
			void play();
			void prepare_kickoff_friendly();
			void execute_kickoff_friendly();
			void prepare_kickoff_enemy();
			void execute_kickoff_enemy();
			void prepare_penalty_friendly();
			void execute_penalty_friendly();
			void prepare_penalty_enemy();
			void execute_penalty_enemy();
			void execute_direct_free_kick_friendly();
			void execute_indirect_free_kick_friendly();
			void execute_direct_free_kick_enemy();
			void execute_indirect_free_kick_enemy();
	};

	/**
	 * A factory for constructing \ref STPStrategy "STPStrategies".
	 */
	class STPStrategyFactory : public StrategyFactory {
		public:
			STPStrategyFactory();
			~STPStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of STPStrategyFactory.
	 */
	STPStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		//		PlayType::PLAY,
	};

	STPStrategyFactory::STPStrategyFactory() : StrategyFactory("STP", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	STPStrategyFactory::~STPStrategyFactory() {
	}

	Strategy::Ptr STPStrategyFactory::create_strategy(World &world) const {
		return STPStrategy::create(world);
	}

	StrategyFactory &STPStrategy::factory() const {
		return factory_instance;
	}

	Strategy::Ptr STPStrategy::create(World &world) {
		const Strategy::Ptr p(new STPStrategy(world));
		return p;
	}

	void STPStrategy::halt() {
		play_executor.tick();
	}

	void STPStrategy::stop() {
		play_executor.tick();
	}

	void STPStrategy::play() {
		play_executor.tick();
	}

	void STPStrategy::prepare_kickoff_friendly() {
		play_executor.tick();
	}

	void STPStrategy::execute_kickoff_friendly() {
		play_executor.tick();
	}

	void STPStrategy::prepare_kickoff_enemy() {
		play_executor.tick();
	}

	void STPStrategy::execute_kickoff_enemy() {
		play_executor.tick();
	}

	void STPStrategy::prepare_penalty_friendly() {
		play_executor.tick();
	}

	void STPStrategy::execute_penalty_friendly() {
		play_executor.tick();
	}

	void STPStrategy::prepare_penalty_enemy() {
		play_executor.tick();
	}

	void STPStrategy::execute_penalty_enemy() {
		play_executor.tick();
	}

	void STPStrategy::execute_direct_free_kick_friendly() {
		play_executor.tick();
	}

	void STPStrategy::execute_indirect_free_kick_friendly() {
		play_executor.tick();
	}

	void STPStrategy::execute_direct_free_kick_enemy() {
		play_executor.tick();
	}

	void STPStrategy::execute_indirect_free_kick_enemy() {
		play_executor.tick();
	}

	STPStrategy::STPStrategy(World &world) : Strategy(world), play_executor(world) {
	}

	STPStrategy::~STPStrategy() {
	}
}

