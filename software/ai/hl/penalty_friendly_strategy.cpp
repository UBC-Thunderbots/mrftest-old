#include "ai/flags.h"
#include "ai/hl/defender.h"
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

	const double PENALTY_MARK_LENGTH = 0.45;
	const double RESTRICTED_ZONE_LENGTH = 0.85;
	
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
			void on_player_added(std::size_t);
			void on_player_removing(std::size_t);

			/**
			 * What this strategy is created for.
			 */
			void prepare_penalty_friendly();
			void execute_penalty_friendly();

			void prepare();

			void run_assignment();

			// the players invovled
			std::vector<Player::Ptr> defenders;
			std::vector<Player::Ptr> offenders;
			Player::Ptr kicker;

			AI::HL::Defender defender;
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
		if (world.friendly_team().size() == 0) {
			return;
		}

		prepare();

		const Point shoot_position = Point(0.5 * world.field().length() - PENALTY_MARK_LENGTH - Robot::MAX_RADIUS, 0);

		AI::HL::Tactics::free_move(world, kicker, shoot_position);
	}

	void PenaltyFriendlyStrategy::execute_penalty_friendly() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		prepare();

		AI::HL::Tactics::shoot(world, kicker, 0);
	}

	void PenaltyFriendlyStrategy::prepare() {
		const Field &f = world.field();

		Point waypoints[5];

		waypoints[0] = Point(0.5 * f.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);
		waypoints[1] = Point(0.5 * f.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);
		waypoints[2] = Point(0.5 * f.length() - RESTRICTED_ZONE_LENGTH - 5 * Robot::MAX_RADIUS, 0);

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		for (size_t i = 0; i < offenders.size(); ++i) {
			// move the robots to position
			offenders[i]->move(waypoints[i], (world.ball().position() - players[i]->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
		}

		defender.set_chase(false);
		defender.tick();
	}

	void PenaltyFriendlyStrategy::run_assignment() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		// it is easier to change players every tick?
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		// sort players by distance to own goal
		if (players.size() > 1) {
			std::sort(players.begin() + 1, players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.field().friendly_goal()));
		}

		// defenders
		Player::Ptr goalie = players[0];

		std::size_t ndefenders = 1; // includes goalie

		switch (players.size()) {
			case 5:
			case 4:
				++ndefenders;

			case 3:
			case 2:
				break;
		}

		// clear up
		defenders.clear();
		offenders.clear();
		kicker.reset();

		// start from 1, to exclude goalie
		for (std::size_t i = 1; i < players.size(); ++i) {
			if (i < ndefenders) {
				defenders.push_back(players[i]);
			} else if (!kicker.is()) {
				kicker = players[i];
			} else {
				offenders.push_back(players[i]);
			}
		}

		LOG_INFO(Glib::ustring::compose("player reassignment %1 defenders, %2 offenders", ndefenders, offenders.size()));

		defender.set_players(defenders, goalie);
	}


	Strategy::Ptr PenaltyFriendlyStrategy::create(World &world) {
		const Strategy::Ptr p(new PenaltyFriendlyStrategy(world));
		return p;
	}

	PenaltyFriendlyStrategy::PenaltyFriendlyStrategy(World &world) : Strategy(world), defender(world) {
		world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &PenaltyFriendlyStrategy::on_player_added));
		world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &PenaltyFriendlyStrategy::on_player_removing));
		run_assignment();
	}

	void PenaltyFriendlyStrategy::on_player_added(std::size_t) {
		run_assignment();
	}

	void PenaltyFriendlyStrategy::on_player_removing(std::size_t) {
		run_assignment();
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

