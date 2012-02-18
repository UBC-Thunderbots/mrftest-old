#include "ai/hl/stp/tactic/free_kick_pass.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/tactic/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "ai/hl/stp/action/pivot.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Action;
using namespace AI::HL::W;

namespace {
	class FreeKickPass : public Tactic {
		public:
			FreeKickPass(const World &world) : Tactic(world) {
				state = TO_BALL;
			}

		private:
			enum tactic_state { TO_BALL, ROTATE_BOT, ROTATE_TOP, ROTATE_MID, SHOOT };
			tactic_state state;
			bool done() const {
				return player && player->autokick_fired();
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute() {
				Point dest;
				const double DISTANCE_FROM_BALL = Robot::MAX_RADIUS + 0.25; // Keep robot this far away from the ball
				const double TOLERANCE = 0.1; // Speed tolerance
				const double ROT_ANGLE = 135; // rotate to this angle
				const double ANGLE_TOL = 5.0; // Be within this angle before shooting
				Point player_to_ball = player->position() - world.ball().position();
				Angle to_target = (world.ball().position() - world.field().enemy_goal()).orientation() - player_to_ball.orientation();
				switch(state) {
					// Move towards the ball
					// When robot stops, start rotating below it
					case TO_BALL:
						dest = world.ball().position() - Point(DISTANCE_FROM_BALL, 0);
						move(world, player, dest, Point(0, 0));
						
						if (player_to_ball.len() - DISTANCE_FROM_BALL < TOLERANCE && player->velocity().len() < TOLERANCE) {
							state = ROTATE_BOT;
						}
						break;
					// Rotate below to a 45 degree angle
					case ROTATE_BOT:
						pivot(world, player, world.ball().position() + Point(0, DISTANCE_FROM_BALL), DISTANCE_FROM_BALL);						
						if (player_to_ball.orientation() >= Angle::of_degrees(-ROT_ANGLE) && player_to_ball.orientation() <= Angle::ZERO) {
							state = ROTATE_TOP;
						}
						break;
					// Rotate above to a 45 degree angle
					case ROTATE_TOP:
						pivot(world, player,world.ball().position() - Point(0, DISTANCE_FROM_BALL), DISTANCE_FROM_BALL);
						if (player_to_ball.orientation() <= Angle::of_degrees(ROT_ANGLE) && player_to_ball.orientation() >= Angle::ZERO) {
							state = ROTATE_MID;
						}
						break;
					// Rotate to shooting position
					case ROTATE_MID:
						pivot(world, player, world.field().enemy_goal(), DISTANCE_FROM_BALL);
						if (to_target.abs() <= Angle::of_degrees(ANGLE_TOL)) {
							state = SHOOT;
						}
						break;
					case SHOOT:
						dest = world.ball().position();
						move(world, player, dest, Point(0, 0));
						player->autokick(AI::HL::STP::BALL_MAX_SPEED);
						break;
				}
			}
			Glib::ustring description() const {
				return "free kick pass";
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::free_kick_pass(const World &world) {
	Tactic::Ptr p(new FreeKickPass(world));
	return p;
}
