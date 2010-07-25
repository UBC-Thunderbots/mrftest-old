#include "ai/coach/coach.h"
#include <cstdlib>

namespace {
	/**
	 * A Coach that picks randomly from the available \ref Strategy "Strategies"
	 * for a given play type.
	 */
	class RandomCoach : public Coach {
		public:
			CoachFactory &get_factory() const;
			void tick();
			Gtk::Widget *get_ui_controls();
			static Coach::Ptr create(const World::Ptr &world);

		private:
			RandomCoach(const World::Ptr &world);
			~RandomCoach();
	};

	/**
	 * A factory for constructing \ref RandomCoach "RandomCoaches".
	 */
	class RandomCoachFactory : public CoachFactory {
		public:
			RandomCoachFactory();
			~RandomCoachFactory();
			Coach::Ptr create_coach(const World::Ptr &world) const;
	};

	/**
	 * The global instance of RandomCoachFactory.
	 */
	RandomCoachFactory factory;

	CoachFactory &RandomCoach::get_factory() const {
		return factory;
	}

	void RandomCoach::tick() {
		// If there is no Strategy or if it has resigned, choose a new one.
		if (!get_strategy() || (get_strategy() && get_strategy()->has_resigned())) {
			const std::vector<StrategyFactory *> &factories = Coach::get_strategies_by_play_type(world->playtype());
			if (factories.empty()) {
				clear_strategy();
			} else {
				set_strategy(factories[std::rand() % factories.size()]);
			}
		}

		// If there is a Strategy, run it.
		if (get_strategy()) {
			get_strategy()->tick();
		}
	}

	Gtk::Widget *RandomCoach::get_ui_controls() {
		return 0;
	}

	Coach::Ptr RandomCoach::create(const World::Ptr &world) {
		const Coach::Ptr p(new RandomCoach(world));
		return p;
	}

	RandomCoach::RandomCoach(const World::Ptr &world) : Coach(world) {
	}

	RandomCoach::~RandomCoach() {
	}

	RandomCoachFactory::RandomCoachFactory() : CoachFactory("Random") {
	}

	RandomCoachFactory::~RandomCoachFactory() {
	}

	Coach::Ptr RandomCoachFactory::create_coach(const World::Ptr &world) const {
		return RandomCoach::create(world);
	}
}

