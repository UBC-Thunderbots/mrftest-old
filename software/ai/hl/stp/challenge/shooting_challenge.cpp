#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/tactic/shooting_challenge.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move.h"
#include "geom/util.h"
#include "util/param.h"
#include "ai/common/playtype.h"


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

				if (world.playtype() == AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) {
					auto shooter = Tactic::shooting_challenge(world, 2.0);
					shooter->set_player(friendly.get(0));
					shooter->execute();
				}
				else if (world.playtype() == AI::Common::PlayType::STOP) {
					auto shooter = Tactic::move(world, Point(0,0));
					shooter->set_player(friendly.get(0));
					shooter->execute();
				}

				else if (world.playtype() == AI::Common::PlayType::HALT) {
					return;
				}


			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_shoot(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(ShootingChallenge)

