#include "ai/hl/stp/tactic/pass_simple.h"
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
#include "ai/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Coordinate;

using AI::HL::STP::min_pass_dist;

namespace {
	struct PasserSimple : public Tactic {
		bool kick_attempted;

		// HYSTERISIS
		Player target;

		explicit PasserSimple(World world) : Tactic(world, true), kick_attempted(false) {
		}

		bool done() const {
			return player && kick_attempted && player.autokick_fired();
		}

		void player_changed() {
			target = Evaluation::select_passee(world);
		}

		bool fail() const {
			if (!target) {
				return false;
			}
			if (!Evaluation::passee_suitable(world, target)) {
				return true;
			}
			return false;
		}

		Player select(const std::set<Player> &players) const {
			// if a player attempted to shoot, keep the player
			if (kick_attempted && players.count(player)) {
				return player;
			}
			return select_baller(world, players, player);
		}

		void execute() {
			if (!target) {
				LOG_ERROR(u8"no target");
				// should fail
				return;
			}

			if (Action::shoot_pass(world, player, target)) {
				kick_attempted = true;
			}

			player.flags(0);
		}

		Glib::ustring description() const {
			return u8"passer-simple";
		}
	};

	struct PasseeSimple : public Tactic {
		const unsigned number;

		explicit PasseeSimple(World world, unsigned number) : Tactic(world, false), number(number) {
		}

		Player select(const std::set<Player> &players) const {
			// hysterysis.
			if (player && players.count(player) && Evaluation::passee_suitable(world, player)) {
				return player;
			}
			for (auto it = players.begin(); it != players.end(); ++it) {
				if (Evaluation::passee_suitable(world, *it)) {
					return *it;
				}
			}
			// return a random player..
			return *players.begin();
		}

		void execute() {
			if (Evaluation::passee_suitable(world, player)) {
				Action::move(world, player, player.position());
			} else {
				// move to a suitable position
				auto dest = AI::HL::STP::Evaluation::offense_positions();
				Action::move(world, player, dest[number]);
				player.prio(AI::Flags::MovePrio::LOW);
			}
		}

		Glib::ustring description() const {
			return u8"passee-simple";
		}
	};

	struct FollowBaller : public Tactic {
		explicit FollowBaller(World world) : Tactic(world, false) {
		}

		Player select(const std::set<Player> &players) const {
			Player best;
			double min_dist = 1e99;
			for (auto it = players.begin(); it != players.end(); ++it) {
				Point dest = Evaluation::calc_fastest_grab_ball_dest_if_baller_shoots(world, (*it).position());
				if (!best || min_dist > (dest - (*it).position()).len()) {
					min_dist = (dest - (*it).position()).len();
					best = *it;
				}
			}
			return best;
		}

		void execute() {
			if (Evaluation::passee_suitable(world, player)) {
				Action::move(world, player, player.position());
			} else {
				Point dest = Evaluation::calc_fastest_grab_ball_dest_if_baller_shoots(world, player.position());
				Action::move(world, player, dest);
			}
		}

		Glib::ustring description() const {
			return u8"follow-baller";
		}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_simple(World world) {
	Tactic::Ptr p(new PasserSimple(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_simple(World world, unsigned number) {
	Tactic::Ptr p(new PasseeSimple(world, number));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::follow_baller(World world) {
	Tactic::Ptr p(new FollowBaller(world));
	return p;
}

