#include "ai/hl/stp/tactic/block_shot_stop.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "geom/util.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/play/play.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
using namespace AI::HL::STP::Play::Global;
using AI::HL::STP::Coordinate;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;


namespace {
	class BlockShotStop final : public Tactic {
		public:
	  explicit BlockShotStop(World world) : Tactic(world) {
			}

		private:
			Player select(const std::set<Player> &players) const override;

			void execute(caller_t& ca) override;

			Glib::ustring description() const override {
				return u8"block shot stop";
			}
	};

	//Define Class player
	//Compare the distance from the 'dest' and return the the player who has the shortest distance
	Player BlockShotStop::select(const std::set<Player> &players) const {
	  Point dest;
		dest = (world.ball().position() + world.field().friendly_goal()) / 2;
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest));
	}

	void BlockShotStop::execute(caller_t& ca) {
		while (true) {
			Point dest;
			dest = (world.ball().position() + world.field().friendly_goal()) / 2;
			player().move_move(dest, (dest - player().position()).orientation());
			yield(ca);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::block_shot_stop(World world) {
  Tactic::Ptr p(new BlockShotStop(world));
	return p;
}
