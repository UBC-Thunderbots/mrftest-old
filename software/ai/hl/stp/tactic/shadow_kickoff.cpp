#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include <algorithm>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;

namespace {
	class ShadowKickoff : public Tactic {
		public:
			ShadowKickoff(const World &world, Enemy::Ptr enemy, const Coordinate default_loc) : Tactic(world), enemy(enemy), default_loc(default_loc) {
			}

		private:
			const Enemy::Ptr enemy;
			const Coordinate default_loc;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "shadow kickoff";
			}
	};

	Player::Ptr ShadowKickoff::select(const std::set<Player::Ptr> &players) const {
		Point location_eval;
		if (enemy->evaluate().is()) {
			location_eval = enemy->evaluate()->position();
		} else {
			location_eval = default_loc.position();
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(location_eval));
	}

	void ShadowKickoff::execute() {
		if (enemy->evaluate().is()) {
			// calculate position to block the side enemies from shooting
			Point block_position = line_intersect(enemy->evaluate()->position(), world.field().friendly_goal(), Point(-0.2, 2), Point(-0.2, -2));
			Action::move(world, player, block_position);
		} else {
			Action::move(world, player, default_loc.position());
		}
	}
	
	class ShadowBall : public Tactic {
		public:
			ShadowBall(const World &world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "shadow ball";
			}
	};

	Player::Ptr ShadowBall::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(Point(world.ball().position().x, -world.ball().position().y)));
	}

	void ShadowBall::execute() {
		Action::move(world, player, Point(world.ball().position().x, -world.ball().position().y));
	}
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_kickoff(const World &world, Enemy::Ptr enemy, const Coordinate default_loc) {
	const Tactic::Ptr p(new ShadowKickoff(world, enemy, default_loc));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_ball(const World &world) {
	const Tactic::Ptr p(new ShadowBall(world));
	return p;
}


