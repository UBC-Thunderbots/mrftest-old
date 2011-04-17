#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	
	class VDHLFactory : public HighLevelFactory {
		public:
			VDHLFactory() : HighLevelFactory("Victory Dance") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	VDHLFactory factory_instance;

	class VDHL : public HighLevel {
		public:
			VDHL(World &world) : world(world) {
			}

			VDHLFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();
				
				for (uint robotIndex = 0; robotIndex < friendly.size(); robotIndex++) {
					Point des (-1.5,0);
					double radius = 0.2 * robotIndex + 0.2;
					double offset_angle = 0.3 + robotIndex*1.1;
					Player::Ptr runner = friendly.get(robotIndex);
					Point diff = (des - friendly.get(0)->position()).rotate(offset_angle);
					Point dest = des - radius * (diff / diff.len());
					
					runner->move(dest, angle_mod((des - runner->position()).orientation()), 0, AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::HIGH);
				}
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context>) {
			}
			
		private:
			World &world;
	};

	HighLevel::Ptr VDHLFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new VDHL(world));
		return p;
	}
}

