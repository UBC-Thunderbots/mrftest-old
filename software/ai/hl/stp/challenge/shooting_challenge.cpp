#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/tactic/shooting_challenge.h"
#include "geom/util.h"
#include "util/param.h"

namespace Flags = AI::Flags;

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;


namespace {
	class ShootingChallenge : public HighLevel {
		public:
			ShootingChallenge(World world) : world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {

				tick_eval(world);

				FriendlyTeam friendly = world.friendly_team();
				if (friendly.size() == 0) {
					return;
				}

				auto shooter = Tactic::shooting_challenge(world, 2.0);
				shooter->set_player(friendly.get(0));
				shooter->execute();
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_shoot(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(ShootingChallenge)
