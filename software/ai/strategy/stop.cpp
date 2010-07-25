#include "ai/strategy/strategy.h"

namespace {
	/**
	 * Manages the robots during a stoppage in place (that is, when the game is
	 * in PlayType::STOP).
	 */
	class StopStrategy : public Strategy {
		public:
			StrategyFactory &get_factory() const;
			void stop();
			static Strategy::Ptr create(const World::Ptr &world);

		private:
			StopStrategy(const World::Ptr &world);
			~StopStrategy();
			void on_play_type_changed();
	};

	/**
	 * A factory for constructing \ref StopStrategy "StopStrategies".
	 */
	class StopStrategyFactory : public StrategyFactory {
		public:
			StopStrategyFactory();
			~StopStrategyFactory();
			Strategy::Ptr create_strategy(const World::Ptr &world) const;
	};

	/**
	 * The global instance of StopStrategyFactory.
	 */
	StopStrategyFactory factory;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::STOP,
	};

	StrategyFactory &StopStrategy::get_factory() const {
		return factory;
	}

	void StopStrategy::stop() {
#warning TODO something sensible
	}

	Strategy::Ptr StopStrategy::create(const World::Ptr &world) {
		const Strategy::Ptr p(new StopStrategy(world));
		return p;
	}

	StopStrategy::StopStrategy(const World::Ptr &world) : Strategy(world) {
		world->signal_playtype_changed.connect(sigc::mem_fun(this, &StopStrategy::on_play_type_changed));
	}

	StopStrategy::~StopStrategy() {
	}

	void StopStrategy::on_play_type_changed() {
		if (world->playtype() != PlayType::STOP) {
			resign();
		}
	}

	StopStrategyFactory::StopStrategyFactory() : StrategyFactory("Stop", HANDLED_PLAY_TYPES, sizeof(HANDLED_PLAY_TYPES) / sizeof(*HANDLED_PLAY_TYPES)) {
	}

	StopStrategyFactory::~StopStrategyFactory() {
	}

	Strategy::Ptr StopStrategyFactory::create_strategy(const World::Ptr &world) const {
		return StopStrategy::create(world);
	}
}

