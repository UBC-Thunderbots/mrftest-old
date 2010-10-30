#include "ai/flags.h"
#include "ai/hl/defender.h"
#include "ai/hl/offender.h"
#include "ai/hl/strategy.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
	/**
	 * Manages the robots during direct and indirect free kicks.
	 */
	class PenaltyFriendlyStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * Creates a new PenaltyFriendlyStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			PenaltyFriendlyStrategy(World &world);
			~PenaltyFriendlyStrategy();

			/**
			 * What this strategy is created for.
			 */
			void prepare_penalty_friendly();
			void execute_penalty_friendly();

			void prepare();

			const static double PENALTY_MARK_LENGTH = 0.45;
			const static double RESTRICTED_ZONE_LENGTH = 0.85;
			const static unsigned int NUMBER_OF_READY_POSITIONS = 4;

			Point ready_positions[NUMBER_OF_READY_POSITIONS];
	};

	/**
	 * A factory for constructing \ref PenaltyFriendlyStrategy "PenaltyFriendlyStrategies".
	 */
	class PenaltyFriendlyStrategyFactory : public StrategyFactory {
		public:
			PenaltyFriendlyStrategyFactory();
			~PenaltyFriendlyStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of PenaltyFriendlyStrategyFactory.
	 */
	PenaltyFriendlyStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PREPARE_PENALTY_FRIENDLY,
		PlayType::EXECUTE_PENALTY_FRIENDLY,
	};

	StrategyFactory &PenaltyFriendlyStrategy::factory() const {
		return factory_instance;
	}

	void PenaltyFriendlyStrategy::prepare_penalty_friendly() {
		prepare();
	}

	void PenaltyFriendlyStrategy::execute_penalty_friendly() {
		prepare();
	}

	/**
	 * Ticks the strategy
	 */
	void PenaltyFriendlyStrategy::prepare() {
		const Field &f = world.field();

		// Let the first robot to be always the shooter
		ready_positions[0] = Point(0.5 * f.length() - PENALTY_MARK_LENGTH - Robot::MAX_RADIUS, 0);

		//ready_positions[1] = Point(0, 0);
		ready_positions[1] = Point(0.5 * f.length() - PENALTY_MARK_LENGTH - 5 * Robot::MAX_RADIUS, 0);

		// Let two robots be on the offensive, in case there is a rebound
		ready_positions[2] = Point(0.5 * f.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);
		ready_positions[3] = Point(0.5 * f.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);

		if (world.friendly_team().size() == 0) {
			return;
		}

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		if (players.size() == 0) {
			LOG_WARN("no robots");
			return;
		}

		if (world.playtype() == PlayType::PREPARE_PENALTY_FRIENDLY) {
			for (size_t i = 1; i < players.size(); ++i) {
				// move the robots to position
				players[i]->move(ready_positions[i - 1], (ready_positions[i - 1] - players[i]->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}
		} else if (world.playtype() == PlayType::EXECUTE_PENALTY_FRIENDLY) {
			// let the shooter shoot
			const Player::Ptr shooter = players[1];
			AI::HL::Tactics::shoot(world, shooter, AI::Flags::FLAG_CLIP_PLAY_AREA);
		} else {
			LOG_WARN("penalty_friendly: unhandled playtype");
			return;
		}
	}

	Strategy::Ptr PenaltyFriendlyStrategy::create(World &world) {
		const Strategy::Ptr p(new PenaltyFriendlyStrategy(world));
		return p;
	}

	PenaltyFriendlyStrategy::PenaltyFriendlyStrategy(World &world) : Strategy(world) {
	}

	PenaltyFriendlyStrategy::~PenaltyFriendlyStrategy() {
	}

	PenaltyFriendlyStrategyFactory::PenaltyFriendlyStrategyFactory() : StrategyFactory("PenaltyFriendly", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	PenaltyFriendlyStrategyFactory::~PenaltyFriendlyStrategyFactory() {
	}

	Strategy::Ptr PenaltyFriendlyStrategyFactory::create_strategy(World &world) const {
		return PenaltyFriendlyStrategy::create(world);
	}
}

