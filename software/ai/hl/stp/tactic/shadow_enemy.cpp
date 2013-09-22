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
			ShadowEnemy(World world, unsigned int index) : Tactic(world), index(index) {
			}

		private:
			Coordinate dest;
			Player select(const std::set<Player> &players) const;
			unsigned int index;
			void execute();
			Glib::ustring description() const {
				return u8"shadow_enemy";
			}
	};

	Player ShadowEnemy::select(const std::set<Player> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest.position()));
	}

	void ShadowEnemy::execute() {
		Point enemy = Enemy::closest_ball(world, index)->evaluate().position();
		Point ball = world.ball().position();
		Point destination = ball - enemy;
		destination = destination.norm() * (AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS);
		destination = ball + destination;
		dest = destination;
		Action::move(world, player, dest.position(), dest.velocity());
		player.dribble_stop();
	}
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_enemy(World world, unsigned int index) {
	Tactic::Ptr p(new ShadowEnemy(world, index));
	return p;
}
