#include "ai/hl/stp/tactic/ball_placement.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "geom/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class BallPlacement final : public Tactic
	{
		public:
			explicit BallPlacement(World world) : Tactic(world, true), move_stop(AI::HL::STP::Tactic::move_stop(world, 0))
			{
			}

		private:
			Tactic::Ptr move_stop;

			bool done() const override;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override
			{
				return u8"ball-placement";
			}
	};

	bool BallPlacement::done() const { return false; }

	Player BallPlacement::select(const std::set<Player> &players) const
	{
		if ((world.ball().position() - world.ball_placement_position()).lensq() < 0.1 * 0.1 && world.ball().velocity().lensq() < 0.1 * 0.1) {
			return move_stop->select(players);
		}
		else {
			return *std::min_element(players.begin(), players.end(),
					AI::HL::Util::CmpDist<Player>(world.ball().position()));
		}
	}

	void BallPlacement::execute()
	{
		player.flags(0);
		player.avoid_distance(AI::Flags::AvoidDistance::MEDIUM);

		if ((world.ball().position() - world.ball_placement_position()).lensq() < 0.1 * 0.1 && world.ball().velocity().lensq() < 0.1 * 0.1) {
			player.flags(AI::Flags::FLAG_AVOID_BALL_TINY);
			move_stop->set_player(player);
			move_stop->execute();
		}
		else {
			if (player.has_ball()) {
				player.mp_dribble(world.ball_placement_position(), (world.ball_placement_position() - player.position()).orientation());
			}
			else {
				Point behind_ball = world.ball().position() - (world.ball_placement_position() - world.ball().position()).norm() * (Robot::MAX_RADIUS + 0.10);

				if (Geom::dist(Geom::Line(behind_ball, world.ball().position()), player.position()) > 0.03 ||
						(world.ball_placement_position() - world.ball().position()).dot(player.position() - world.ball().position()) > 0) {
					player.flags(AI::Flags::FLAG_AVOID_BALL_TINY);
					player.mp_move(behind_ball, (world.ball_placement_position() - world.ball().position()).orientation());
				}
				else {
					player.mp_move(world.ball().position(), (world.ball_placement_position() - world.ball().position()).orientation());
				}
			}
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::ball_placement(World world)
{
	Tactic::Ptr p(new BallPlacement(world));
	return p;
}

