#include "ai/flags.h"
#include "ai/hl/old/defender.h"
#include "ai/hl/old/offender.h"
#include "ai/hl/old/strategy.h"
#include "ai/hl/old/tactics.h"
#include "ai/hl/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
	/**
	 * Free kick friendly.
	 */
	class FreeKickFriendlyStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * Creates a new FreeKickFriendlyStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			FreeKickFriendlyStrategy(World &world);
			~FreeKickFriendlyStrategy();
			void on_player_added(std::size_t);
			void on_player_removed();

			void execute_indirect_free_kick_friendly();
			void execute_direct_free_kick_friendly();

			void execute();

			/**
			 * Dynamic assignment every tick can be bad if players keep changing the roles.
			 */
			void run_assignment();

			// the players invovled
			std::vector<Player::Ptr> defenders;
			std::vector<Player::Ptr> offenders;
			Player::Ptr kicker;

			AI::HL::Defender defender;
			AI::HL::Offender offender;
	};

	/**
	 * A factory for constructing \ref FreeKickFriendlyStrategy "StopStrategies".
	 */
	class FreeKickFriendlyStrategyFactory : public StrategyFactory {
		public:
			FreeKickFriendlyStrategyFactory();
			~FreeKickFriendlyStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of FreeKickFriendlyStrategyFactory.
	 */
	FreeKickFriendlyStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY,
		PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY,
	};

	StrategyFactory &FreeKickFriendlyStrategy::factory() const {
		return factory_instance;
	}

	void FreeKickFriendlyStrategy::execute_direct_free_kick_friendly() {
		execute();
	}

	// a goal may not be scored directly from the kick
	void FreeKickFriendlyStrategy::execute_indirect_free_kick_friendly() {
		execute();
	}

	void FreeKickFriendlyStrategy::execute() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		defender.set_chase(false);
		defender.tick();
		offender.set_chase(false);
		offender.tick();

		// TODO something more sensible

		// look for someone to pass to (offenders only), if nobody is open then just shoot it into the open
		if (kicker.is()) {
			Player::Ptr passee = AI::HL::Util::choose_best_pass(world, offenders);
			if (passee.is()) {
				AI::HL::Tactics::pass(world, kicker, passee, AI::Flags::FLAG_CLIP_PLAY_AREA);
			} else {
				AI::HL::Tactics::shoot(world, kicker, AI::Flags::FLAG_CLIP_PLAY_AREA, 6.0);
			}
		}
	}

	void FreeKickFriendlyStrategy::run_assignment() {
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
		offender.set_players(offenders);
	}

	Strategy::Ptr FreeKickFriendlyStrategy::create(World &world) {
		const Strategy::Ptr p(new FreeKickFriendlyStrategy(world));
		return p;
	}

	FreeKickFriendlyStrategy::FreeKickFriendlyStrategy(World &world) : Strategy(world), defender(world), offender(world) {
		world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &FreeKickFriendlyStrategy::on_player_added));
		world.friendly_team().signal_robot_removed().connect(sigc::mem_fun(this, &FreeKickFriendlyStrategy::on_player_removed));
		run_assignment();
	}

	void FreeKickFriendlyStrategy::on_player_added(std::size_t) {
		run_assignment();
	}

	void FreeKickFriendlyStrategy::on_player_removed() {
		run_assignment();
	}

	FreeKickFriendlyStrategy::~FreeKickFriendlyStrategy() {
	}

	FreeKickFriendlyStrategyFactory::FreeKickFriendlyStrategyFactory() : StrategyFactory("FreeKick Friendly", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	FreeKickFriendlyStrategyFactory::~FreeKickFriendlyStrategyFactory() {
	}

	Strategy::Ptr FreeKickFriendlyStrategyFactory::create_strategy(World &world) const {
		return FreeKickFriendlyStrategy::create(world);
	}
}

