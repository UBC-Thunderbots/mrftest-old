#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/test/test.h"
#include "geom/util.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	IntParam pass_target_param("passing target points", "STP/test_pass", 0, 0, 14);
	DoubleParam negligible_velocity("velocity to ignore", "STP/test_pass", 0.1, 0.0, 1.0);

	DegreeParam passer_tol_target(" angle tolerance that the passer needs to be with respect to the target (degrees)", "STP/test_pass", 30.0, 0.0, 180.0);
	DegreeParam passer_tol_reciever(" angle tolerance that the passer needs to be with respect to the passee (degrees)", "STP/test_pass", 20.0, 0.0, 180.0);
	DoubleParam passee_tol(" distance tolerance that the passee needs to be with respect to the passer shot", "STP/test_pass", 0.05, 0.0, 1.0);

	DoubleParam passee_hack_dist("Hack to get reciever to move more quickly to intercept pos by modifying dest (meters)", "STP/test_pass", 0.03, 0.0, 1.0);
	// make better targets

	BoolParam lower_number_passer("lower numbered bot is passer", "STP/test_pass", true);

	const Point default_targets[] = {
		Point(-1.2, 0),
		Point(1.5, 0),
		Point(1.2, 0.3),
		Point(1.2, -0.3),
		Point(1.2, 0),
		Point(1.2, -0.3),
		Point(1.2, 0),
		Point(0.5, 0),
		Point(2.5, 0),
		Point(0.5, 1.2),
		Point(1, -0.6),
		Point(2, 0.6),
		Point(1, -0.6),
		Point(0.5, 0),
		Point(2.5, 0.6)
	};

	const int default_targets_n = G_N_ELEMENTS(default_targets);

	class TestPass : public HighLevel {
		public:
			TestPass(World world) : world(world), targets(default_targets, default_targets + default_targets_n) {
				kicked_count = 0;
				kicked = false;
			}

		private:
			World world;

			std::vector<Point> targets;

			// the position of the passing robot before/during the kick
			Point player_kick_pos;

			bool kicked;

			int kicked_count;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				const std::size_t pass_target = static_cast<std::size_t>(pass_target_param);

				tick_eval(world);

				std::vector<AI::HL::W::Player> players = AI::HL::Util::get_players(world.friendly_team());
				if (players.size() != 2) {
					return;
				}

				if (!lower_number_passer) {
					std::reverse(players.begin(), players.end());
				}

				if (kicked) {
					kicked_count++;
				} else {
					kicked_count = 0;
				}
				if (kicked_count > 50) {
					kicked = false;
				}

				if (kicked) {
					Action::move(players[0], players[0].orientation(), players[0].position());
				} else if (!players[0].has_ball()) {
					Action::intercept_pivot(world, players[0], targets[pass_target]);
				}

				// passer shoots
				if (players[0].has_ball()) {
					if (Action::shoot_pass(world, players[0], targets[pass_target])) {
						kicked = true;
					}
				}

				bool fast_ball = world.ball().velocity().len() > negligible_velocity;
				if (Evaluation::player_within_angle_thresh(players[0], targets[pass_target], passer_tol_target) || (kicked && !fast_ball)) {
					Point pass_dir(100, 0);
					pass_dir = pass_dir.rotate(players[0].orientation());

					Point intercept_pos = closest_lineseg_point(players[1].position(), players[0].position(), players[0].position() + pass_dir);
					Point addit = passee_hack_dist * (intercept_pos - players[0].position()).norm();

					Action::move(players[1], (players[0].position() - intercept_pos).orientation(), intercept_pos + addit);
				} else if (kicked && fast_ball) {
					Point intercept_pos = closest_lineseg_point(players[1].position(), world.ball().position(), world.ball().position() + 100 * (world.ball().velocity().norm()));
					//Point pass_dir = (world.ball().position() - players[0].position()).norm();

					Point addit = passee_hack_dist * (intercept_pos - players[1].position()).norm();
					Action::move(players[1], (players[0].position() - intercept_pos).orientation(), intercept_pos + addit);
				} else {
					// passee move to target
					Action::move(players[1], (world.ball().position() - players[1].position()).orientation(), targets[pass_target]);
				}

				players[1].type(AI::Flags::MoveType::DRIBBLE);
			}
	};
}

HIGH_LEVEL_REGISTER(TestPass)

