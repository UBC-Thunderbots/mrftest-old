#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"

#include <cassert>
#include "../action/defend.h"
#include "defend.h"

#include "../action/goalie.h"
#include "../action/repel.h"

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
			void execute(caller_t& ca);
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
			void execute(caller_t& ca) override;
			Player select(const std::set<Player> &) const override {
				assert(false);
			}
			Glib::ustring description() const override {
				return u8"goalie (helped by defender)";
			}
	};

	class Defender final : public Tactic {
		public:
			explicit Defender(World world, unsigned i, bool active_baller) : Tactic(world), index(i), active_baller(active_baller) {
			}

		private:
			unsigned index;
			Player select(const std::set<Player> &players) const override;
			bool active_baller;
			void execute(caller_t& ca) override;
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

	void Goalie2::execute(caller_t& ca) {
		while(true) {
			for (auto i : world.enemy_team()) {
			// If enemy is in our defense area, go touch them so we get penalty kick
				if(AI::HL::Util::point_in_friendly_defense(world.field() , i.position())) {
					player().avoid_distance(AI::Flags::AvoidDistance::SHORT);
					Action::goalie_move(ca, world, player(), i.position());
					return;
				}
			}

			if (tdefend_goalie) {
				Vector2 dirToGoal = (world.field().friendly_goal() - world.ball().position()).norm();
				Vector2 dest = world.field().friendly_goal() - (1.3 * Robot::MAX_RADIUS * dirToGoal);
				Action::goalie_move(ca, world, player(), dest);
			} else if (dangerous(world, player())){
				AI::HL::STP::Action::lone_goalie(ca, world, player());
			} else if (world.friendly_team().size() > defender_role + 1) {
				// has defender
				// auto waypoints = Evaluation::evaluate_defense();
				// Action::goalie_move(world, player, waypoints[0]);
				const Field &field = world.field();
				const Point goal_side = Point(-field.length() / 2, field.goal_width() / 2);
				const Point goal_opp = Point(-field.length() / 2, -field.goal_width() / 2);


				Point block_pos = calc_block_cone(goal_side , goal_opp , player().position() , Robot::MAX_RADIUS);

				Vector2 dirToGoal = (world.field().friendly_goal() - block_pos).norm();
				Vector2 dest = block_pos - (1.3 * Robot::MAX_RADIUS * dirToGoal);
				Action::goalie_move(ca, world, player(), dest);
			} else {
				// solo
				AI::HL::STP::Action::lone_goalie(ca, world, player());
			}
			yield(ca);
		}

	}


	void Goalie::execute(caller_t& ca) {
		while(true) {
			auto waypoints = Evaluation::evaluate_defense();
			Vector2 dest = waypoints[0];
			if (tdefend_goalie) {
				AI::HL::STP::Action::lone_goalie(ca, world, player());
				return;
			} else if (dangerous(world, player())){
				AI::HL::STP::Action::lone_goalie(ca, world, player());
			}
			Action::goalie_move(ca, world, player(), dest);
			yield(ca);
		}
	}

	Player Defender::select(const std::set<Player> &players) const {
		Vector2 dest = calc_defend_pos(index);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest));
	}

	void Defender::execute(caller_t& ca) {
		Vector2 dest = calc_defend_pos(index);

		Action::defender_move(ca, world, player(), dest, active_baller);
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

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_defender(AI::HL::W::World world, bool active_baller) {
	Tactic::Ptr p(new Defender(world, 1, active_baller));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra1(AI::HL::W::World world, bool active_baller) {
	Tactic::Ptr p(new Defender(world, 2, active_baller));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra2(AI::HL::W::World world, bool active_baller) {
	Tactic::Ptr p(new Defender(world, 3, active_baller));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra3(AI::HL::W::World world, bool active_baller) {
	Tactic::Ptr p(new Defender(world, 4, active_baller));
	return p;
}
