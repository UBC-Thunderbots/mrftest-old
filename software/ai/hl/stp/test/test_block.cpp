#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/action/block.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestBlockFactory : public HighLevelFactory {
		public:
			TestBlockFactory() : HighLevelFactory("Test STP Block") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestBlockFactory factory_instance;

	class TestBlock : public HighLevel {
		public:
			TestBlock(World &world) : world(world) {
			}

		private:
			World &world;

			TestBlockFactory &factory() const {
				return factory_instance;
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);
				FriendlyTeam &friendly = world.friendly_team();
				const EnemyTeam &enemy = world.enemy_team();
				if (friendly.size() == 0 || enemy.size() == 0) {
					return;
				}

				Action::block_goal(world, friendly.get(0), enemy.get(0));
			}
	};

	HighLevel::Ptr TestBlockFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestBlock(world));
		return p;
	}
}

