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

			Offender offender;
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
		PlayType::PLAY,
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

		offender.set_chase(true);
		offender.tick();
	}

	FranticPlayStrategy::FranticPlayStrategy(World &world) : Strategy(world), offender(world) {
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

		if (world.friendly_team().size() == 0) {
			LOG_WARN("no players");
			return;
		}

		// it is easier to change players every tick?
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		LOG_INFO(Glib::ustring::compose("defenders are for sissies: %2 offenders", players.size()));
		offender.set_players(players);

	}

}

