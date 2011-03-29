#include "ai/common/playtype.h"
#include "ai/hl/hl.h"
#include "ai/hl/old/strategy.h"
#include "util/property.h"
#include <cassert>
#include <cstdlib>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	const std::vector<StrategyFactory *> &get_strategies_by_play_type(AI::Common::PlayType::PlayType pt) {
		static bool initialized = false;
		static std::vector<StrategyFactory *> vectors[PlayType::COUNT];

		assert(pt >= 0);
		assert(pt < PlayType::COUNT);

		if (!initialized) {
			typedef StrategyFactory::Map Map;
			const Map &m = StrategyFactory::all();
			for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
				StrategyFactory *factory = i->second;
				for (std::size_t j = 0; j < factory->handled_play_types_size; ++j) {
					assert(factory->handled_play_types[j] >= 0);
					assert(factory->handled_play_types[j] < AI::Common::PlayType::COUNT);
					vectors[factory->handled_play_types[j]].push_back(factory);
				}
			}
			initialized = true;
		}

		return vectors[pt];
	}

	class OldHLFactory : public HighLevelFactory {
		public:
			OldHLFactory() : HighLevelFactory("Old AI") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	OldHLFactory factory_instance;

	class OldHL : public HighLevel {
		public:
			OldHL(World &world) : world(world), strategy(Strategy::Ptr()), strategy_label("Strategy:") {
				strategy_entry.set_editable(false);
				box.pack_start(strategy_label, Gtk::PACK_SHRINK);
				box.pack_start(strategy_entry, Gtk::PACK_EXPAND_WIDGET);
				strategy.signal_changed().connect(sigc::mem_fun(this, &OldHL::on_strategy_changed));
				on_strategy_changed();
			}

			OldHLFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				// If there is no strategy, choose one.
				Strategy::Ptr strat = strategy;
				if (!strat.is() || strat->has_resigned()) {
					choose_new_strategy();
					strat = strategy;
				}

				// Run the current strategy, if any.
				if (strat.is() && !strat->has_resigned()) {
					strat->tick();
				}
			}

			Gtk::Widget *ui_controls() {
				return &box;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context>) {
			}

		private:
			World &world;
			Property<Strategy::Ptr> strategy;
			Gtk::HBox box;
			Gtk::Label strategy_label;
			Gtk::Entry strategy_entry;

			void on_strategy_changed() {
				Strategy::Ptr strat = strategy;
				if (strat.is()) {
					strategy_entry.set_text(strat->factory().name());
				} else {
					strategy_entry.set_text("<None>");
				}
			}

			void choose_new_strategy() {
				const std::vector<StrategyFactory *> &factories = get_strategies_by_play_type(world.playtype());
				if (factories.empty()) {
					strategy = Strategy::Ptr();
				} else {
					strategy = factories[std::rand() % factories.size()]->create_strategy(world);
				}
			}
	};

	HighLevel::Ptr OldHLFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new OldHL(world));
		return p;
	}
}

