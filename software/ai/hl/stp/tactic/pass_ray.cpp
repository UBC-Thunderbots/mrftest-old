#include "ai/hl/stp/tactic/pass_ray.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
using AI::HL::STP::min_pass_dist;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	DegreeParam small_pass_ray_angle(u8"Small ray shoot rotation (degrees)", u8"STP/PassRay", 20, 0, 180);

	struct PasserRay final : public Tactic {
		bool kick_attempted;
		Player target;

		// HYSTERESIS
		Angle ori_fix;

		explicit PasserRay(World world) : Tactic(world, true), kick_attempted(false) {
		}

		bool done() const override {
			return player && kick_attempted && player.autokick_fired();
		}

		void player_changed() override {
			ori_fix = Evaluation::best_shoot_ray(world, player).second;
		}

		bool fail() const override {
			Angle ori = Evaluation::best_shoot_ray(world, player).second;
			if (ori.angle_diff(ori_fix) > small_pass_ray_angle) {
				return true;
			}
			return false;
		}

		Player select(const std::set<Player> &players) const override {
			// if a player attempted to shoot, keep the player
			if (kick_attempted && players.count(player)) {
				return player;
			}
			return select_baller(world, players, player);
		}

		void execute() override {
			Angle ori = Evaluation::best_shoot_ray(world, player).second;

			Point target = player.position() + 10 * Point::of_angle(ori);
			if (Action::shoot_target(world, player, target, Action::pass_speed)) {
				kick_attempted = true;
			}
		}

		Glib::ustring description() const override {
			return u8"passer-ray";
		}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_ray(World world) {
	Tactic::Ptr p(new PasserRay(world));
	return p;
}

