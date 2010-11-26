#include "ai/coach/coach.h"
#include <gtkmm.h>

namespace {
	/**
	 * A coach that allows the user to select a single strategy in a UI.
	 */
	class SingleStrategyCoach : public AI::Coach::Coach {
		public:
			SingleStrategyCoach(AI::Coach::W::World &world);
			~SingleStrategyCoach();
			AI::Coach::CoachFactory &factory() const;
			void tick();
			Gtk::Widget *ui_controls();

		private:
			Gtk::ComboBoxText selector;
			bool force_change;

			void on_selector_changed();
	};

	/**
	 * A factory for creating SingleStrategyCoach instances.
	 */
	class SingleStrategyCoachFactory : public AI::Coach::CoachFactory {
		public:
			SingleStrategyCoachFactory();
			~SingleStrategyCoachFactory();
			AI::Coach::Coach::Ptr create_coach(AI::Coach::W::World &world) const;
	};
}

SingleStrategyCoachFactory single_strategy_coach_factory;

namespace {
	SingleStrategyCoach::SingleStrategyCoach(AI::Coach::W::World &world) : AI::Coach::Coach(world), force_change(false) {
		const AI::HL::StrategyFactory::Map &m = AI::HL::StrategyFactory::all();
		selector.append_text("<Select Strategy>");
		selector.set_active_text("<Select Strategy>");
		for (AI::HL::StrategyFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
			selector.append_text(i->second->name());
		}
		selector.signal_changed().connect(sigc::mem_fun(this, &SingleStrategyCoach::on_selector_changed));
	}

	SingleStrategyCoach::~SingleStrategyCoach() {
	}

	AI::Coach::CoachFactory &SingleStrategyCoach::factory() const {
		return single_strategy_coach_factory;
	}

	void SingleStrategyCoach::tick() {
		AI::HL::Strategy::Ptr strategy = world.strategy();
		if (!strategy.is() || strategy->has_resigned() || force_change) {
			const Glib::ustring &sel = selector.get_active_text();
			const AI::HL::StrategyFactory::Map::const_iterator iter = AI::HL::StrategyFactory::all().find(sel.collate_key());
			if (iter != AI::HL::StrategyFactory::all().end()) {
				world.strategy(iter->second);
			}
			force_change = false;
		}
	}

	Gtk::Widget *SingleStrategyCoach::ui_controls() {
		return &selector;
	}

	void SingleStrategyCoach::on_selector_changed() {
		force_change = true;
	}

	SingleStrategyCoachFactory::SingleStrategyCoachFactory() : AI::Coach::CoachFactory("Single Strategy Coach") {
	}

	SingleStrategyCoachFactory::~SingleStrategyCoachFactory() {
	}

	AI::Coach::Coach::Ptr SingleStrategyCoachFactory::create_coach(AI::Coach::W::World &world) const {
		AI::Coach::Coach::Ptr p(new SingleStrategyCoach(world));
		return p;
	}
}

