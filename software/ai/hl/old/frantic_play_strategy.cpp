#include "ai/hl/old/defender.h"
#include "ai/hl/old/offender.h"
#include "ai/hl/old/strategy.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using AI::HL::Defender;
using AI::HL::Offender;
using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
	const unsigned int NORMAL_SCORE = 6; // if the combined score is below this level the players will play normally

	const unsigned int FRANTIC_SCORE = 9; // else if the combined score is below this level the players will play more frantically

	/**
	 * A full implementation of a strategy that handles normal play.
	 * This is basically the same as basic play but can act frantically
	 * ie. it evaluates the game base on the scoring of the two teams and balance between offenders and defenders accordingly.
	 */
	class FranticPlayStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * The reason we have this class.
			 */
			void play();

			static Strategy::Ptr create(AI::HL::W::World &world);

		private:
			FranticPlayStrategy(AI::HL::W::World &world);
			~FranticPlayStrategy();
			void on_player_added(std::size_t);
			void on_player_removed();

			/**
			 * Recalculates and redo all assignments.
			 */
			void run_assignment();
			void calc_chaser();

			Player::Ptr goalie;
			Offender offender;
			Defender defender;

			std::vector<Player::Ptr> offenders;
			std::vector<Player::Ptr> defenders; // normally this should be empty
	};

	/**
	 * A factory for constructing \ref FranticPlayStrategy "BasicPlayStrategies".
	 */
	class FranticPlayStrategyFactory : public StrategyFactory {
		public:
			FranticPlayStrategyFactory();
			~FranticPlayStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of FranticPlayStrategyFactory.
	 */
	FranticPlayStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		// PlayType::PLAY,
	};

	FranticPlayStrategyFactory::FranticPlayStrategyFactory() : StrategyFactory("Frantic Play", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	FranticPlayStrategyFactory::~FranticPlayStrategyFactory() {
	}

	Strategy::Ptr FranticPlayStrategyFactory::create_strategy(World &world) const {
		return FranticPlayStrategy::create(world);
	}

	StrategyFactory &FranticPlayStrategy::factory() const {
		return factory_instance;
	}

	Strategy::Ptr FranticPlayStrategy::create(World &world) {
		const Strategy::Ptr p(new FranticPlayStrategy(world));
		return p;
	}

	void FranticPlayStrategy::play() {
		if (world.friendly_team().size() == 0) {
			return;
		}
		if (offenders.size() > 0) {
			offender.tick();
		}
		// there might be no defenders but with one existing goalie
		if (defenders.size() > 0 || goalie.is()) {
			defender.tick();
		}
	}

	FranticPlayStrategy::FranticPlayStrategy(World &world) : Strategy(world), offender(world), defender(world) {
		world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &FranticPlayStrategy::on_player_added));
		world.friendly_team().signal_robot_removed().connect(sigc::mem_fun(this, &FranticPlayStrategy::on_player_removed));
		run_assignment();
	}

	FranticPlayStrategy::~FranticPlayStrategy() {
	}

	void FranticPlayStrategy::on_player_added(std::size_t) {
		run_assignment();
	}

	void FranticPlayStrategy::on_player_removed() {
		run_assignment();
	}

	void FranticPlayStrategy::run_assignment() {
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
		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

		// check the sum of the score of both teams (so you are losing big or unlikely winning big)
		// if sum of the score of both teams are low (so team should be not too frantic or crazy yet)
		// should probably have one goalie and the rest offenders
		// NOTE: code for restricting goalie to have accompanying defenders are only commented out for now

		// However, if enemy robots are all "clustered" in your side of the field
		// ie. the enemy are launching a major offensive,
		// you should probably behave frantically by having all defenders and no offenders
		// check this by checking the # of enemy robots (3+?) on your side of the field

		int enemy_cnt = 0;
		for (std::size_t i = 0; i < enemies.size(); ++i) {
			// simply check the enemies' positions
			if (enemies[i]->position().x < 0) {
				enemy_cnt++;
			}
		}

		if (enemy_cnt > 2) {
			// all out defensive
			goalie = players[0];
			for (std::size_t i = 1; i < players.size(); ++i) {
				defenders.push_back(players[i]);
			}
			defender.set_players(defenders, goalie);
		} else if (world.friendly_team().score() + world.enemy_team().score() < NORMAL_SCORE) {
			// standard play of 2 def, 2 off, 1 goalie
			goalie = players[0];
			std::size_t ndefenders = 1; // includes goalie
			switch (players.size()) {
				case 5:
					++ndefenders;

				case 4:
				case 3:
					++ndefenders;

				case 2:
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
			offender.set_players(offenders);
			defender.set_players(defenders, goalie);
		} else if (world.friendly_team().score() + world.enemy_team().score() < FRANTIC_SCORE) {
			// 1 goalie, 4 off
			goalie = players[0];
			for (std::size_t i = 1; i < players.size(); ++i) {
				offenders.push_back(players[i]);
			}
			offender.set_players(offenders);
			defender.set_players(defenders, goalie);
		} else {
			// all out offensive
			for (std::size_t i = 0; i < players.size(); ++i) {
				offenders.push_back(players[i]);
			}
			offender.set_players(offenders);
		}

		LOG_INFO(Glib::ustring::compose("defenders are for sissies: %1 defenders, %2 offenders", defenders.size(), offenders.size()));
	}

	void FranticPlayStrategy::calc_chaser() {
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
			if (AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position())) {
				offender_chase = false;
			}
			// double dist = (goalie->position() - world.ball().position()).len();
			// if (dist < best_dist) {
			// offender_chase = false;
			// }
		}

		offender.set_chase(offender_chase);
		defender.set_chase(!offender_chase);
	}
}

