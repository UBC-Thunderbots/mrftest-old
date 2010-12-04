#include "ai/flags.h"
#include "ai/hl/strategy.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "uicomponents/param.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {

	BoolParam STAY_OWN_HALF("Test strategy stay own half", true);
	/**
	 * Manages the robots for testing purposes
	 */
	class TestStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;
			void play();

			/**
			 * Creates a new TestStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			TestStrategy(World &world);
			~TestStrategy();
	};

	/**
	 * A factory for constructing \ref TestStrategy "TestStrategies".
	 */
	class TestStrategyFactory : public StrategyFactory {
		public:
			TestStrategyFactory();
			~TestStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of TestStrategyFactory.
	 */
	TestStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PLAY,
	};

	StrategyFactory &TestStrategy::factory() const {
		return factory_instance;
	}




	void TestStrategy::play() {
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		unsigned int flags = 0;
		
		if (STAY_OWN_HALF) {
			flags |= AI::Flags::FLAG_STAY_OWN_HALF;
		}

		if (players.size() == 0) {
			return;
		}

		for (std::vector<Player::Ptr>::iterator it = players.begin(); it != players.end(); it++) {
			(*it)->move(world.ball().position(), (*it)->orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
		}

	}

	Strategy::Ptr TestStrategy::create(World &world) {
		const Strategy::Ptr p(new TestStrategy(world));
		return p;
	}

	TestStrategy::TestStrategy(World &world) : Strategy(world) {
	}

	TestStrategy::~TestStrategy() {
	}

	TestStrategyFactory::TestStrategyFactory() : StrategyFactory("TEST", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	TestStrategyFactory::~TestStrategyFactory() {
	}

	Strategy::Ptr TestStrategyFactory::create_strategy(World &world) const {
		return TestStrategy::create(world);
	}
}

