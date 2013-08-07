#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "ai/hl/world.h"

using AI::Common::PlayType;
using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	DoubleParam pos_dis_threshold_sd(u8"pos distance threshold shooting def", u8"NC", 0.05, 0, 1.0);

	class ShootingDefChallenge : public HighLevel {
		public:
			ShootingDefChallenge(World world) : world(world), time_steps(0) {
				for (int i = 0 ; i < 5 ; i++) done[i] = 0;
			}

			HighLevelFactory &factory() const;

			void tick() {

				const Field &f = world.field();
				/** 
				 * 0 - goalie
				 * 1-4 defenders
				 */

				double x1 = f.friendly_goal().x+f.defense_area_radius()+ Robot::MAX_RADIUS;

				const std::pair<Point, Angle> tasks[5][4] = {
					{std::make_pair(Point(f.friendly_goal().x+ Robot::MAX_RADIUS, -0.2), Angle::zero()),
					std::make_pair(Point(f.friendly_goal().x+ Robot::MAX_RADIUS, 0), Angle::zero()),
					std::make_pair(Point(f.friendly_goal().x+ Robot::MAX_RADIUS, 0.2), Angle::zero()),
					std::make_pair(Point(f.friendly_goal().x+ Robot::MAX_RADIUS, 0), Angle::zero())},

					{std::make_pair(Point(x1, -0.6-Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1-f.defense_area_radius()/2, -1.0-Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(f.friendly_goal().x+ Robot::MAX_RADIUS, -1.4-Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1-f.defense_area_radius()/2, -1.0-Robot::MAX_RADIUS), Angle::zero())},
					{std::make_pair(Point(x1, -0.4-Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1, -0.2-Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1, 0-Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1, -0.2-Robot::MAX_RADIUS), Angle::zero())},
					{std::make_pair(Point(x1, 0.4+Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1, 0.2+Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1, 0+Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1, 0.2+Robot::MAX_RADIUS), Angle::zero())},
					{std::make_pair(Point(x1, 0.6+Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1-f.defense_area_radius()/2, 1.0+Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(f.friendly_goal().x+ Robot::MAX_RADIUS, 1.4+Robot::MAX_RADIUS), Angle::zero()),
					std::make_pair(Point(x1-f.defense_area_radius()/2, 1.0+Robot::MAX_RADIUS), Angle::zero())},
				};

				FriendlyTeam friendly = world.friendly_team();

				if (friendly.size() !=5) {
					return;
				}

				Player defenders[] = {friendly[0], friendly[1], friendly[2], friendly[3], friendly[4]};

				if (!defenders[0] || !defenders[1] || !defenders[2] || !defenders[3] || !defenders[4]) {
					return;
				}

				for (std::size_t i = 0 ; i < 5 ; i++){
					const Point diff_pos = defenders[i].position() - tasks[i][done[i]%4].first;

					if (diff_pos.len() < pos_dis_threshold_sd) {
						++done[i];
					}

					// set avoidance distance based on whether the robot is moving
					for (Robot j : world.enemy_team()) {
						j.avoid_distance(AI::Flags::AvoidDistance::MEDIUM);
					}

					//defenders[i].flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE);
					defenders[i].type(AI::Flags::MoveType::NORMAL);
					defenders[i].prio(AI::Flags::MovePrio::HIGH);
					defenders[i].move(tasks[i][done[i]%4].first, (world.ball().position() - defenders[i].position()).orientation(), Point());
				}
			}

			Gtk::Widget *ui_controls() {
				return nullptr;
			}

		private:
			World world;
			int time_steps;
			std::size_t done[5];
	};
}

HIGH_LEVEL_REGISTER(ShootingDefChallenge)

