#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/testshootspeed.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "geom/angle.h"

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {


	class TestShootSpeed final : public Tactic {
		public:
			explicit TestShootSpeed(World world, bool force) : Tactic(world, true), kick_attempted(false), force(force), shoot_score(Angle::zero()){}

		private: 
			bool kick_attempted;
			bool force;
			Angle shoot_score;
			
			bool done() const override;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override 
			{
				return u8"shoot-goal";		
			}
	};


	Player TestShootSpeed::select(const std::set<Player> &players) const {
		// if a player attempted to shoot, keep the player
		if (!kick_attempted && players.count(player)) {
			return player;
		}
		return select_baller(world, players, player);
	}

	bool TestShootSpeed::done() const {
		return player /* && kick_attempted*/ && player.autokick_fired();
	}

	void TestShootSpeed::execute() {
		if (AI::HL::STP::Action::shoot_goal(world, player, force)) {
			kick_attempted = true;
		}
		Angle cur_shoot_score = AI::HL::STP::Evaluation::get_shoot_score(world, player);
//		if (new_shoot && ((cur_shoot_score < shoot_score + Angle::of_radians(1e-9) && shoot_score > Angle::zero()) || cur_shoot_score > shoot_thresh)) {
			player.autokick(BALL_MAX_SPEED);
//		}
		shoot_score = cur_shoot_score;
	}

}


Tactic::Ptr AI::HL::STP::Tactic::test_shoot_speed(World world, bool force) {
	Tactic::Ptr p(new TestShootSpeed(world, force));
	return p;
}


