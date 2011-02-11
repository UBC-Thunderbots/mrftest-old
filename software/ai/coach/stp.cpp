#include "ai/coach/coach.h"
#include <cstdlib>

using AI::Coach::Coach;
using AI::Coach::CoachFactory;
using AI::HL::Strategy;
using namespace AI::Coach::W;

namespace {
	/**
	 * A Coach that picks STPly from the available \ref Strategy "Strategies" for a given play type.
	 * This coach also never forcefully removes a Strategy from play.
	 * It only acts when the current Strategy resigns.
	 */
	class STPCoach : public Coach {
		public:
			CoachFactory &factory() const;
			void tick();
			Gtk::Widget *ui_controls();
			static Coach::Ptr create(World &world);

		private:
			STPCoach(World &world);
			~STPCoach();
	};

	/**
	 * A factory for constructing \ref STPCoach "STPCoaches".
	 */
	class STPCoachFactory : public CoachFactory {
		public:
			STPCoachFactory();
			~STPCoachFactory();
			Coach::Ptr create_coach(World &world) const;
	};

	/**
	 * The global instance of STPCoachFactory.
	 */
	STPCoachFactory factory_instance;

	CoachFactory &STPCoach::factory() const {
		return factory_instance;
	}

	void STPCoach::tick() {
		// If there is no Strategy or if it has resigned, choose a new one.
		Strategy::Ptr strategy = world.strategy();
		if (!strategy.is() || strategy->has_resigned()) {
			const AI::HL::StrategyFactory::Map::const_iterator iter = AI::HL::StrategyFactory::all().find("STP");
			world.strategy(iter->second);
		}
	}

	Gtk::Widget *STPCoach::ui_controls() {
		return 0;
	}

	Coach::Ptr STPCoach::create(World &world) {
		const Coach::Ptr p(new STPCoach(world));
		return p;
	}

	STPCoach::STPCoach(World &world) : Coach(world) {
	}

	STPCoach::~STPCoach() {
	}

	STPCoachFactory::STPCoachFactory() : CoachFactory("STP") {
	}

	STPCoachFactory::~STPCoachFactory() {
	}

	Coach::Ptr STPCoachFactory::create_coach(World &world) const {
		return STPCoach::create(world);
	}
}

