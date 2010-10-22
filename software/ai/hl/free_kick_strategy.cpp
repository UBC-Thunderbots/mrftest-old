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
	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = 0.050 + Robot::MAX_RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

	DoubleParam separation_angle("angle to separate players (degrees)", 5, 0, 20);

	/**
	 * Manages the robots during a stoppage in place (that is, when the game is in PlayType::STOP).
	 */
	class FreeKickStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * Creates a new FreeKickStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			FreeKickStrategy(World &world);
			~FreeKickStrategy();

			void execute_indirect_free_kick_friendly();
			void execute_direct_free_kick_friendly();
			void execute_indirect_free_kick_enemy();
			void execute_direct_free_kick_enemy();

			void prepare();

			AI::HL::Defender defender;
			AI::HL::Offender offender;
	};

	/**
	 * A factory for constructing \ref FreeKickStrategy "StopStrategies".
	 */
	class FreeKickStrategyFactory : public StrategyFactory {
		public:
			FreeKickStrategyFactory();
			~FreeKickStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of FreeKickStrategyFactory.
	 */
	FreeKickStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY,
		PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY,
		PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY,
		PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY,
	};

	StrategyFactory &FreeKickStrategy::factory() const {
		return factory_instance;
	}

	void FreeKickStrategy::execute_direct_free_kick_friendly() {
		prepare();
	}


//a goal may not be scored directly from the kick
	void FreeKickStrategy::execute_indirect_free_kick_friendly() {
		prepare();
	}

	void FreeKickStrategy::execute_direct_free_kick_enemy() {
		prepare();
	}


//a goal may not be scored directly from the kick
	void FreeKickStrategy::execute_indirect_free_kick_enemy() {
		prepare();
	}


	void FreeKickStrategy::prepare() {

		if (world.friendly_team().size() == 0) {
			return;
		}

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		if (players.size() == 0) {
			return;
		}
		// run assignment and tick
		if (world.playtype() == PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) {

		} else if (world.playtype() == PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY){
				Player::Ptr goalie = players[4];
				players.pop_back();

		//500 mm from the ball

		} else if (world.playtype() == PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY || world.playtype() == PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY){
                	int preferred_offender_number = std::max(1, static_cast<int>(world.friendly_team().size()) - 3);

				//decrease the number of offenders by 1, if there are no offenders, set it to one
			        preferred_offender_number--;
			        if (preferred_offender_number < 1)
			            preferred_offender_number = 1;

			        // preferred_defender_number includes goalie
			        int preferred_defender_number = world.friendly_team().size() - preferred_offender_number;

				Player::Ptr goalie = players[4];
				players.pop_back();

		}  else {
			LOG_ERROR("freeKick: unhandled playtype");
		}

	}

	Strategy::Ptr FreeKickStrategy::create(World &world) {
		const Strategy::Ptr p(new FreeKickStrategy(world));
		return p;
	}

	FreeKickStrategy::FreeKickStrategy(World &world) : Strategy(world), defender(world), offender(world) {	}

	FreeKickStrategy::~FreeKickStrategy() {
	}

	FreeKickStrategyFactory::FreeKickStrategyFactory() : StrategyFactory("FreeKick", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	FreeKickStrategyFactory::~FreeKickStrategyFactory() {
	}

	Strategy::Ptr FreeKickStrategyFactory::create_strategy(World &world) const {
		return FreeKickStrategy::create(world);
	}
}

