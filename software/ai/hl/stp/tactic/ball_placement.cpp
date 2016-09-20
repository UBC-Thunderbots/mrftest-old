#include "ai/hl/stp/tactic/ball_placement.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/action/legacy_shoot.h"
#include "ai/hl/stp/action/legacy_catch.h"
#include "ai/hl/stp/action/legacy_move.h"
#include "ai/hl/stp/action/legacy_dribble.h"
#include "ai/hl/stp/action/legacy_action.h"
#include "ai/hl/util.h"
#include "geom/util.h"

#include "util/dprint.h"

#include "ai/hl/stp/tactic/legacy_tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace LegacyAction = AI::HL::STP::LegacyAction;

namespace {
	class BallPlacement final : public LegacyTactic
	{
		public:
			explicit BallPlacement(World world) : LegacyTactic(world, true)
			{
			}

		private:
			bool done() const override;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override
			{
				return u8"ball-placement";
			}
	};

	bool BallPlacement::done() const {
		return player && (world.ball().position() - world.ball_placement_position()).lensq() < 0.1 * 0.1 && world.ball().velocity().lensq() < 0.1 * 0.1 && (player.position() - world.ball().position()).len() > 0.5;
	}

	Player BallPlacement::select(const std::set<Player> &players) const
	{
		return *std::min_element(players.begin(), players.end(),
				AI::HL::Util::CmpDist<Player>(world.ball().position()));
	}

	void BallPlacement::execute()
	{
		if ((world.ball().position() - world.ball_placement_position()).len() > 0.08) {
			if (!player.has_ball()) {
				Action::legacy_catch_ball(world, player, world.ball_placement_position());
			}
			else {
				Action::dribble(world, player, world.ball().position(), (world.ball_placement_position() - world.ball().position()).orientation());
			}
		}
		else if ((player.position() - world.ball().position()).len() < 0.5) {
			Action::move(world, player, world.ball().position() + (player.position() - world.ball().position()).norm() * 0.6);
		}
		else {
			LegacyAction::clear_legacy_prim(player);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::ball_placement(World world)
{
	Tactic::Ptr p(new BallPlacement(world));
	return p;
}

