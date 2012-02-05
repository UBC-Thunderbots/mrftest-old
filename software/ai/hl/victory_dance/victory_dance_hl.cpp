#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	class VDHL : public HighLevel {
		public:
			VDHL(World &world) : world(world) {
			}

			HighLevelFactory &factory() const;

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();

				for (uint robotIndex = 0; robotIndex < friendly.size(); robotIndex++) {
					Point des(-1.5, 0);
					double radius = 0.2 * robotIndex + 0.2;
					Angle offset_angle = Angle::of_radians(0.7 + robotIndex * 1.1);
					Player::Ptr runner = friendly.get(robotIndex);
					Point diff = (des - friendly.get(0)->position()).rotate(offset_angle);
					Point dest = des - radius * (diff / diff.len());

					runner->flags(0);
					runner->type(AI::Flags::MoveType::NORMAL);
					runner->prio(AI::Flags::MovePrio::HIGH);
					runner->move(dest, (des - runner->position()).orientation().angle_mod(), Point());
				}
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			World &world;
	};
}

HIGH_LEVEL_REGISTER(VDHL)

