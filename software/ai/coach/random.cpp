#include "ai/coach/coach.h"
#include <cstdlib>

using AI::Coach::Coach;
using AI::Coach::CoachFactory;
using AI::HL::Strategy;
using namespace AI::Coach::W;

namespace {
	/**
	 * A Coach that picks randomly from the available \ref Strategy "Strategies" for a given play type.
	 * This coach also never forcefully removes a Strategy from play.
	 * It only acts when the current Strategy resigns.
	 */
	class RandomCoach : public Coach {
		public:
			CoachFactory &factory() const;
			void tick();
			Gtk::Widget *ui_controls();
			static Coach::Ptr create(World &world);

		private:
			RandomCoach(World &world);
			~RandomCoach();
	};

	/**
	 * A factory for constructing \ref RandomCoach "RandomCoaches".
	 */
	class RandomCoachFactory : public CoachFactory {
		public:
			RandomCoachFactory();
			~RandomCoachFactory();
			Coach::Ptr create_coach(World &world) const;
	};

	/**
	 * The global instance of RandomCoachFactory.
	 */
	RandomCoachFactory factory_instance;

	CoachFactory &RandomCoach::factory() const {
		return factory_instance;
	}

	void RandomCoach::tick() {
		// If there is no Strategy or if it has resigned, choose a new one.
		Strategy::Ptr strategy = world.strategy();
		if (!strategy.is() || strategy->has_resigned()) {
			const std::vector<AI::HL::StrategyFactory *> &factories = Coach::get_strategies_by_play_type(world.playtype());
			if (factories.empty()) {
				world.strategy(0);
			} else {
				world.strategy(factories[std::rand() % factories.size()]);
			}
		}
	}

	Gtk::Widget *RandomCoach::ui_controls() {
		return 0;
	}

	Coach::Ptr RandomCoach::create(World &world) {
		const Coach::Ptr p(new RandomCoach(world));
		return p;
	}

	RandomCoach::RandomCoach(World &world) : Coach(world) {
	}

	RandomCoach::~RandomCoach() {
	}

	RandomCoachFactory::RandomCoachFactory() : CoachFactory("Random") {
	}

	RandomCoachFactory::~RandomCoachFactory() {
	}

	Coach::Ptr RandomCoachFactory::create_coach(World &world) const {
		return RandomCoach::create(world);
	}
}

