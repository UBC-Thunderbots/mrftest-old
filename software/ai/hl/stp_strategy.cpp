#include "ai/hl/strategy.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/dprint.h"

#include <glibmm.h>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayManager;
using AI::HL::STP::Tactic::Tactic;

namespace {
	// The maximum amount of time a play can be running.
	const double PLAY_TIMEOUT = 30.0;

	/**
	 * STP based strategy.
	 * Please refer to the paper by CMU on
	 * STP: Skills, tactics and plays for multi-robot control in adversarial environments
	 */
	class STPStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			void play();

			void calc_play();
			void calc_tactics();
			void execute_tactics();

			static Strategy::Ptr create(AI::HL::W::World &world);

		private:
			/**
			 * Status of this strategy
			 * 0 = calc play
			 * 1 = calc tactics
			 * 2 = execute tactics
			 */
			int state;

			Play::Ptr current_play;
			PlayManager *current_play_manager;

			// roles
			int current_role_step;
			std::vector<Tactic::Ptr> current_goalie_role;
			std::vector<Tactic::Ptr> current_role1;
			std::vector<Tactic::Ptr> current_role2;
			std::vector<Tactic::Ptr> current_role3;
			std::vector<Tactic::Ptr> current_role4;

			STPStrategy(AI::HL::W::World &world);
			~STPStrategy();
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
		PlayType::PLAY,
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

	void STPStrategy::calc_play() {
		static bool initialized = false;
		static std::vector<PlayManager *> managers;

		if (!initialized) {
			const PlayManager::Map &m = PlayManager::all();
			for (PlayManager::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
				managers.push_back(i->second);
			}
			initialized = true;
		}

		state = 0;

		// find a valid state
		std::random_shuffle(managers.begin(), managers.end());
		for (std::size_t i = 0; i < managers.size(); ++i) {
			if (managers[i]->applicable(world)) {
				current_play_manager = managers[i];
				current_play = current_play_manager->create_play(world);
				LOG_INFO(managers[i]->name());
				break;
			}
		}

		// ensure we have a play manager
		if (current_play_manager == NULL) {
			return;
		}

		state = 1;
	}

	void STPStrategy::calc_tactics() {
		state = 0;

		current_role_step = 0;
		current_goalie_role.clear();
		current_role1.clear();
		current_role2.clear();
		current_role3.clear();
		current_role4.clear();

		current_play->assign(current_goalie_role, current_role1, current_role2, current_role3, current_role4);

		state = 2;
	}

	void STPStrategy::execute_tactics() {
		// check if current roles are done

		return;

		/*
		// do matching
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		std::vector<bool> players_used(players.size(), false);
		std::vector<Player::Ptr> assignment(current_tactics.size());

		for (std::size_t i = 0; i < current_tactics.size(); ++i) {
			double best_score = 0;
			std::size_t best_j = 0;
			Player::Ptr best;
			for (std::size_t j = 0; j < players.size(); ++j) {
				if (players_used[j]) {
					continue;
				}
				double score = current_tactics[i]->score(players[j]);
				if (!best.is() || score > best_score) {
					best = players[j];
					best_j = j;
					best_score = score;
				}
			}
			if (best.is()) {
				players_used[best_j] = true;
				assignment[i] = best;
			}
		}

		bool active_assigned = false;

		// now run the current_tactics
		for (std::size_t i = 0; i < current_tactics.size(); ++i) {
			if (!assignment[i].is()) {
				continue;
			}
			current_tactics[i]->set_player(assignment[i]);
			current_tactics[i]->execute();
			if (current_tactics[i] == current_active_tactic) {
				active_assigned = true;
			}
		}

		if (active_assigned == false) {
			state = 0;
		}

		// check if task is done
		if (current_active_tactic->done()) {
			LOG_INFO("tactic done");
			state = 1;
		}

		// check if it wants to terminate
		if (current_play->has_resigned()) {
			LOG_INFO("play resigned");
			state = 0;
		}
		*/
	}

	void STPStrategy::play() {
		// check if current play wants to continue
		if (state != 0 && !current_play->done()) {
			state = 0;
		}

		// check if current is valid
		if (state == 0) {
			calc_play();
			if (state == 0) {
				LOG_WARN("calc play failed");
				return;
			}
		}

		// check if current is valid
		if (state == 1) {
			calc_tactics();
			if (state == 0) {
				LOG_WARN("calc tactics failed");
				return;
			}
		}

		execute_tactics();
	}

	STPStrategy::STPStrategy(World &world) : Strategy(world), current_play_manager(NULL) {
		state = 0;
	}

	STPStrategy::~STPStrategy() {
	}
}

