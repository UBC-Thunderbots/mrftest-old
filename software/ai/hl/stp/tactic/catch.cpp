#include <algorithm>

#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/tactic/catch.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/util.h"

namespace Primitives = AI::BE::Primitives;
namespace Evaluation = AI::HL::STP::Evaluation;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class CatchBall final : public Tactic {
		public:
			explicit CatchBall(World world) : Tactic(world) {
			}

		private:
			void execute(caller_t& ca) override;
			Player select(const std::set<Player> &players) const override;

			Glib::ustring description() const override {
				return u8"catch-ball";
			}
	};

	Player CatchBall::select(const std::set<Player> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpScalar<Player, double>(
					[this](Player p){ return (p.position() - Evaluation::baller_catch_position(world, p)).len(); }
		));
	}

	void CatchBall::execute(caller_t& ca) {
		AI::HL::STP::Action::catch_ball(ca, world, player(), world.field().enemy_goal());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::catch_ball(World world) {
	Tactic::Ptr p(new CatchBall(world));
	return p;
}
