#include "ai/hl/strategy.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/dprint.h"

#include <glibmm.h>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

using AI::HL::STP::Play;
using AI::HL::STP::PlayManager;
using AI::HL::STP::Tactic;

namespace {

	/**
	 * STP based strategy.
	 * Please refer to the paper by CMU on
	 * STP: Skills, tactics and plays for multi-robot control in adversarial environments
	 */
	class STPStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			void play();

			void reset();

			static Strategy::Ptr create(AI::HL::W::World &world);

		private:
			Play::Ptr current_play;
			PlayManager* current_play_manager;

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

	void STPStrategy::reset() {
		static bool initialized = false;
		static std::vector<PlayManager*> managers;

		if (!initialized) {
			const PlayManager::Map &m = PlayManager::all();
			for (PlayManager::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
				managers.push_back(i->second);
			}
			initialized = true;
		}

		current_play_manager = NULL;
		current_play.reset();

		std::random_shuffle(managers.begin(), managers.end());
		for (std::size_t i = 0; i < managers.size(); ++i) {
			if (managers[i]->score(world) > 0) {
				current_play_manager = managers[i];
				current_play = current_play_manager->create_play(world);

				LOG_INFO(current_play->name());

				break;
			}
		}
	}

	void STPStrategy::play() {

		// check if current is valid
		if (current_play_manager == NULL || current_play_manager->score(world) == 0) {
			reset();
		}

		if (current_play_manager == NULL) {
			LOG_WARN("did not find suitable play");
			return;
		}

		// get the tactics
		std::vector<Tactic::Ptr> tactics = current_play->tick();

		// make sure we have 5 tactics
		if (tactics.size() != 5) {
			LOG_ERROR("Play did not return 5 tactics!");
			current_play.reset();
			current_play_manager = NULL;
			return;
		}

		// make sure all tactics are assigned
		for (std::size_t i = 0; i < tactics.size(); ++i) {
			if (!tactics[i].is()) {
				LOG_ERROR("Play did not assign all 5 tactics!");
				current_play.reset();
				current_play_manager = NULL;
				return;
			}
		}

		// do matching
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		std::vector<bool> players_used(players.size(), false);
		std::vector<Player::Ptr> assignment(tactics.size());
		for (std::size_t i = 0; i < tactics.size(); ++i) {
			double best_score = 0;
			std::size_t best_j = 0;
			Player::Ptr best;
			for (std::size_t j = 0; j < players.size(); ++j) {
				if (players_used[j]) continue;
				double score = tactics[i]->score(players[j]);
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

		// now run the tactics
		for (std::size_t i = 0; i < tactics.size(); ++i) {
			if (!assignment[i].is()) continue;
			tactics[i]->tick(assignment[i]);
		}

		// check if it wants to terminate
		if (current_play->has_resigned()) {
			// do something
			current_play.reset();
			current_play_manager = NULL;
		}
	}

	STPStrategy::STPStrategy(World &world) : Strategy(world), current_play_manager(NULL) {
	}

	STPStrategy::~STPStrategy() {
	}

}

