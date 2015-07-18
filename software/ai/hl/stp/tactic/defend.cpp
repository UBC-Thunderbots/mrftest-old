#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/defend.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/util.h"
#include "geom/util.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace Geom;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;

namespace {
	BoolParam tdefend_goalie(u8"Whether or not Terence Defense should take the place of normal goalie", u8"AI/HL/STP/Tactic/defend", false);
	BoolParam tdefend_defender1(u8"Whether or not Terence Defense should take the place of normal defender 1", u8"AI/HL/STP/Tactic/defend", false);
	BoolParam tdefend_defender2(u8"Whether or not Terence Defense should take the place of normal defender 2", u8"AI/HL/STP/Tactic/defend", false);
	BoolParam tdefend_defender3(u8"Whether or not Terence Defense should take the place of normal defender 3", u8"AI/HL/STP/Tactic/defend", false);

	/**
	 * Goalie in a team of N robots.
	 */
	class Goalie2 final : public Tactic {
		public:
			explicit Goalie2(World world, size_t defender_role) : Tactic(world), defender_role(defender_role) {
			}

		private:
			size_t defender_role;
			void execute();
			Player select(const std::set<Player> &) const override {
				assert(false);
			}
			Glib::ustring description() const override {
				if (world.friendly_team().size() > defender_role + 1) {
					return u8"goalie-dynamic duo";
				} else {
					return u8"goalie-dynamic lone";
				}
			}
	};

	/**
	 * Goalie in a team of N robots.
	 */
	class Goalie final : public Tactic {
		public:
			explicit Goalie(World world) : Tactic(world) {
			}

		private:
			void execute() override;
			Player select(const std::set<Player> &) const override {
				assert(false);
			}
			Glib::ustring description() const override {
				return u8"goalie (helped by defender)";
			}
	};

	class Defender final : public Tactic {
		public:
			explicit Defender(World world, unsigned i) : Tactic(world), index(i) {
			}

		private:
			unsigned index;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Vector2 calc_defend_pos(unsigned index) const;
			Glib::ustring description() const override {
				return u8"extra defender";
			}
	};

	bool dangerous(World world, const Player &player) {
		// definition of "danger" is identified by the seg point between ball, net and players
		const double danger_dist = 0.3;
		// definition of "danger" is identified by the distance from ball to net
		const double danger_dist_goal = 1.0;

		// check if a ball is too close
		if ((world.ball().position() - world.field().friendly_goal()).len() < danger_dist_goal) {
			return true;
		}
		// check if there are any defenders close by
		for (const Player i : world.friendly_team()) {
			bool close_to_block_formation = dist(Seg(world.ball().position(), world.field().friendly_goal()), i.position()) < danger_dist;
			bool goalie = i.position().close(player.position(), 0.1);
			if (close_to_block_formation && !goalie)
				return false;
		}
		return true;
	}

	void Goalie2::execute() {
		if (tdefend_goalie) {
			Vector2 dirToGoal = (world.field().friendly_goal() - world.ball().position()).norm();
			Vector2 dest = world.field().friendly_goal() - (2 * Robot::MAX_RADIUS * dirToGoal);
			Action::goalie_move(world, player, dest);
		} else if (dangerous(world, player)){
			AI::HL::STP::Action::lone_goalie(world, player);
		} else if (world.friendly_team().size() > defender_role + 1) {
			// has defender
			auto waypoints = Evaluation::evaluate_defense();
			Action::goalie_move(world, player, waypoints[0]);
		} else {
			// solo
			AI::HL::STP::Action::lone_goalie(world, player);
		}
	}

	void Goalie::execute() {
		auto waypoints = Evaluation::evaluate_defense();
		Vector2 dest = waypoints[0];
		if (tdefend_goalie) {
			AI::HL::STP::Action::lone_goalie(world, player);
			return;
		} else if (dangerous(world, player)){
			AI::HL::STP::Action::lone_goalie(world, player);
		}
		Action::goalie_move(world, player, dest);
	}

	Player Defender::select(const std::set<Player> &players) const {
		Vector2 dest = calc_defend_pos(index);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest));
	}

	void Defender::execute() {
		Vector2 dest = calc_defend_pos(index);
		Action::defender_move(world, player, dest);
	}

	Vector2 Defender::calc_defend_pos(unsigned index) const {
		auto waypoints = Evaluation::evaluate_defense();
		Vector2 dest = waypoints[index];
		// tdefend switch
		if (tdefend_defender1 && index == 1) {
			dest = Evaluation::evaluate_tdefense(world, index);
		} else if (tdefend_defender2 && index == 2) {
			dest = Evaluation::evaluate_tdefense(world, index);
		} else if (tdefend_defender3 && index == 3) {
			dest = Evaluation::evaluate_tdefense(world, index);
		}
		return dest;
	}
}

Tactic::Ptr AI::HL::STP::Tactic::goalie_dynamic(World world, const size_t defender_role) {
	Tactic::Ptr p(new Goalie2(world, defender_role));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_goalie(AI::HL::W::World world) {
	Tactic::Ptr p(new Goalie(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_defender(AI::HL::W::World world) {
	Tactic::Ptr p(new Defender(world, 1));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra1(AI::HL::W::World world) {
	Tactic::Ptr p(new Defender(world, 2));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra2(AI::HL::W::World world) {
	Tactic::Ptr p(new Defender(world, 3));
	return p;
}


