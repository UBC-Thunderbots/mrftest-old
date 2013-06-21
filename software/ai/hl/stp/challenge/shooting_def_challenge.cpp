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
	class ShootingDefChallenge : public HighLevel {
		public:
			ShootingDefChallenge(World world) : world(world), time_steps(0) {
				for (int i = 0 ; i < 4 ; i++) done[i] = 0;
			}

			HighLevelFactory &factory() const;

			void tick() {

				const Field &f = world.field();
				const std::pair<Point, Angle> tasks[5][4] = {
					{std::make_pair(Point(-f.length()/2+(0.1+Robot::MAX_RADIUS), f.width()/2-(0.1+Robot::MAX_RADIUS)), Angle::ZERO),
					std::make_pair(Point(0, 0), Angle::ZERO),
					std::make_pair(Point(f.length()/2-(0.1+Robot::MAX_RADIUS), -f.width()/2+(0.1+Robot::MAX_RADIUS)), Angle::ZERO)},
					{std::make_pair(f.penalty_friendly(), Angle::ZERO),
					std::make_pair(Point(0, 0), Angle::ZERO),
					std::make_pair(f.penalty_enemy(), Angle::ZERO)},
					{std::make_pair(Point(-f.length()/2+(0.1+Robot::MAX_RADIUS), -f.width()/2+(0.1+Robot::MAX_RADIUS)), Angle::ZERO),
					std::make_pair(Point(0, 0), Angle::ZERO),
					std::make_pair(Point(f.length()/2-(0.1+Robot::MAX_RADIUS), f.width()/2-(0.1+Robot::MAX_RADIUS)), Angle::ZERO)},
					{std::make_pair(Point(-f.length()/2+(0.1+Robot::MAX_RADIUS), -f.width()/2+(0.1+Robot::MAX_RADIUS)), Angle::ZERO),
					std::make_pair(Point(0, 0), Angle::ZERO),
					std::make_pair(Point(f.length()/2-(0.1+Robot::MAX_RADIUS), f.width()/2-(0.1+Robot::MAX_RADIUS)), Angle::ZERO)},
					{std::make_pair(Point(-f.length()/2+(0.1+Robot::MAX_RADIUS), -f.width()/2+(0.1+Robot::MAX_RADIUS)), Angle::ZERO),
					std::make_pair(Point(0, 0), Angle::ZERO),
					std::make_pair(Point(f.length()/2-(0.1+Robot::MAX_RADIUS), f.width()/2-(0.1+Robot::MAX_RADIUS)), Angle::ZERO)}
				};

				FriendlyTeam friendly = world.friendly_team();

				if (friendly.size() !=4 || !(world.playtype() == PlayType::PLAY)) {
					return;
				}

				Player defenders[] = {friendly.get(0), friendly.get(1), friendly.get(2), friendly.get(3)};

				if (!defenders[0] || !defenders[1] || !defenders[2] || !defenders[3]) {
					return;
				}

				for (std::size_t i = 0 ; i < 4 ; i++){
					const Point diff_pos = defenders[i].position() - tasks[i][done[i]%4].first;

					if (diff_pos.len() < pos_dis_threshold_nav) {
						if (done[i] == 0) {
							time_steps = 0;
						}
						++done[i];
					}

					// set avoidance distance based on whether the robot is moving
					for (std::size_t j = 0; j < world.enemy_team().size(); ++j) {
						world.enemy_team().get(i).avoid_distance(AI::Flags::AvoidDistance::MEDIUM);
					}

					defenders[i].flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE);
					defenders[i].type(AI::Flags::MoveType::NORMAL);
					defenders[i].prio(AI::Flags::MovePrio::HIGH);
					defenders[i].move(tasks[i][done[i]%4].first, tasks[i][done[i]%4].second, Point());
				}
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			World world;
			std::vector<std::pair<Point, Angle> > tasks[4];
			int time_steps;
			std::size_t done[4];
	};
}

HIGH_LEVEL_REGISTER(ShootingDefChallenge)

