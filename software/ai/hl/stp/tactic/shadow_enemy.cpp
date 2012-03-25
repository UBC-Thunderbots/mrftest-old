#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "ai/hl/stp/action/move.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
using AI::HL::STP::Coordinate;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;


namespace {
	class ShadowEnemy : public Tactic {
		public:
			ShadowEnemy(const World &world) : Tactic(world) {
			}

		private:
			Coordinate dest;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;

			void execute();
			Glib::ustring description() const {
				return "shadow_enemy";
			}
	};

	Player::Ptr ShadowEnemy::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest.position()));
	}

	void ShadowEnemy::execute() {
		Point enemy = Enemy::closest_ball(world, 0)->evaluate()->position();
		Point ball = world.ball().position();
		Point destination = ball - enemy;
		destination = destination.norm() * (AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS);
		destination = ball + destination;
		dest = destination;
		Action::move(world, player, dest.position(), dest.velocity());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_enemy(const World &world) {
	Tactic::Ptr p(new ShadowEnemy(world));
	return p;
}
