#include "ai/hl/strategy.h"
#include "ai/hl/defender.h"
#include "ai/hl/offender.h"
#include "ai/hl/util.h"
#include "util/dprint.h"

#include <glibmm.h>

using AI::HL::Defender;
using AI::HL::Offender;
using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
	/**
	 * A full implementation of a strategy that handles normal play.
	 */
	class BasicPlayStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * The reason we have this class.
			 */
			void play();

			static Strategy::Ptr create(AI::HL::W::World &world);

		private:
			BasicPlayStrategy(AI::HL::W::World &world);
			~BasicPlayStrategy();
			void on_player_added(std::size_t);
			void on_player_removing(std::size_t);

			/**
			 * Recalculates and redo all assignments.
			 */
			void run_assignment();

			void calc_chaser();

			Player::Ptr goalie;
			Defender defender;
			Offender offender;

			std::vector<Player::Ptr> defenders; // excludes goalie
			std::vector<Player::Ptr> offenders;
	};

	/**
	 * A factory for constructing \ref BasicPlayStrategy "BasicPlayStrategies".
	 */
	class BasicPlayStrategyFactory : public StrategyFactory {
		public:
			BasicPlayStrategyFactory();
			~BasicPlayStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of BasicPlayStrategyFactory.
	 */
	BasicPlayStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PLAY,
	};

	BasicPlayStrategyFactory::BasicPlayStrategyFactory() : StrategyFactory("Basic Play", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	BasicPlayStrategyFactory::~BasicPlayStrategyFactory() {
	}

	Strategy::Ptr BasicPlayStrategyFactory::create_strategy(World &world) const {
		return BasicPlayStrategy::create(world);
	}

	StrategyFactory &BasicPlayStrategy::factory() const {
		return factory_instance;
	}

	Strategy::Ptr BasicPlayStrategy::create(World &world) {
		const Strategy::Ptr p(new BasicPlayStrategy(world));
		return p;
	}

	void BasicPlayStrategy::play() {
		if (world.friendly_team().size() == 0) {
			return;
		}
		
		//TODO set passee for defender

		calc_chaser();
		offender.tick();
		defender.tick();
	}

	BasicPlayStrategy::BasicPlayStrategy(World &world) : Strategy(world), defender(world), offender(world) {
		world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &BasicPlayStrategy::on_player_added));
		world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &BasicPlayStrategy::on_player_removing));
		run_assignment();
	}

	BasicPlayStrategy::~BasicPlayStrategy() {
	}

	void BasicPlayStrategy::on_player_added(std::size_t) {
		run_assignment();
	}

	void BasicPlayStrategy::on_player_removing(std::size_t) {
		run_assignment();
	}

	void BasicPlayStrategy::run_assignment() {
		// clear up
		goalie.reset();
		defenders.clear();
		offenders.clear();

		if (world.friendly_team().size() == 0) {
			LOG_WARN("no players");
			return;
		}

		// it is easier to change players every tick?
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		// sort players by distance to own goal
		if (players.size() > 1) {
			std::sort(players.begin() + 1, players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.field().friendly_goal()));
		}

		// defenders
		goalie = players[0];

		std::size_t ndefenders = 1; // includes goalie
		// int noffenders = 0;

		switch (players.size()) {
			case 5:
				++ndefenders;

			case 4:
			// ++noffenders;

			case 3:
				++ndefenders;

			case 2:
				// ++noffenders;
				break;
		}

		// start from 1, to exclude goalie
		for (std::size_t i = 1; i < players.size(); ++i) {
			if (i < ndefenders) {
				defenders.push_back(players[i]);
			} else {
				offenders.push_back(players[i]);
			}
		}

		LOG_INFO(Glib::ustring::compose("player reassignment %1 defenders, %2 offenders", ndefenders, offenders.size()));

		offender.set_players(offenders);
		defender.set_players(defenders, goalie);
	}

	void BasicPlayStrategy::calc_chaser() {
		// see who has the closest ball
		bool offender_chase = true;
		double best_dist = 1e99;
		for (std::size_t i = 0; i < offenders.size(); ++i) {
			best_dist = std::min(best_dist, (offenders[i]->position() - world.ball().position()).len());
		}
		for (std::size_t i = 0; i < defenders.size(); ++i) {
			double dist = (defenders[i]->position() - world.ball().position()).len();
			if (dist < best_dist) {
				offender_chase = false;
				break;
			}
		}
		// goalie special
		{
			if (AI::HL::Util::point_in_friendly_defense(world, world.ball().position())) {
				offender_chase = false;
			}
			//double dist = (goalie->position() - world.ball().position()).len();
			//if (dist < best_dist) {
				//offender_chase = false;
			//}
		}

		offender.set_chase(offender_chase);
		defender.set_chase(!offender_chase);
	}
}

