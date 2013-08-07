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
	DoubleParam pos_dis_threshold_nav("pos distance threshold nav 2013", "NC", 0.05, 0, 1.0);

	class NCHL2013 : public HighLevel {
		public:
			NCHL2013(World world) : world(world), time_steps(0) {
				for (int i = 0 ; i < 3 ; i++) done[i] = 0;
			}

			HighLevelFactory &factory() const;

			void tick() {

				const Field &f = world.field();
				const std::pair<Point, Angle> tasks[3][3] = {
					{std::make_pair(Point(-f.length()/2+(Robot::MAX_RADIUS), f.width()/2-(Robot::MAX_RADIUS)), Angle::zero()),
					std::make_pair(Point(0, 0), Angle::zero()),
					std::make_pair(Point(f.length()/2-(Robot::MAX_RADIUS), -f.width()/2+(Robot::MAX_RADIUS)), Angle::zero())},
					{std::make_pair(f.penalty_friendly(), Angle::zero()),
					std::make_pair(Point(0, 0), Angle::zero()),
					std::make_pair(f.penalty_enemy(), Angle::zero())},
					{std::make_pair(Point(-f.length()/2+(Robot::MAX_RADIUS), -f.width()/2+(Robot::MAX_RADIUS)), Angle::zero()),
					std::make_pair(Point(0, 0), Angle::zero()),
					std::make_pair(Point(f.length()/2-(Robot::MAX_RADIUS), f.width()/2-(Robot::MAX_RADIUS)), Angle::zero())}
				};

				FriendlyTeam friendly = world.friendly_team();

				if (friendly.size() !=3 || !(world.playtype() == PlayType::PLAY || world.playtype() == PlayType::STOP)) {
					return;
				}

				Player runners[] = {friendly[0], friendly[1], friendly[2]};

				if (!runners[0] || !runners[1] || !runners[2]) {
					return;
				}

				if (world.playtype() == PlayType::STOP) {
					for (std::size_t i = 0 ; i < 3 ; i++){
						runners[i].move(tasks[i][0].first, tasks[i][0].second, Point());
						done[i] = 0;
					}
					return;
				}				

				time_steps++;

				for (std::size_t i = 0 ; i < 3 ; i++){
					if (done[i] > 3) {
						continue;
					}

					const Point diff_pos = runners[i].position() - tasks[i][done[i]].first;

					if (diff_pos.len() < pos_dis_threshold_nav) {
						if (done[i] == 0) {
							time_steps = 0;
						}
						++done[i];
					}

					if (done[i] == 3) {
						LOG_INFO(Glib::ustring::compose("time steps taken: %1", time_steps));
						++done[i];
						return;
					}

					// set avoidance distance based on whether the robot is moving
					for (Robot j : world.enemy_team()) {
						j.avoid_distance(AI::Flags::AvoidDistance::MEDIUM);
					}

					runners[i].flags(0);
					runners[i].type(AI::Flags::MoveType::NORMAL);
					runners[i].prio(AI::Flags::MovePrio::HIGH);
					runners[i].move(tasks[i][done[i]].first, tasks[i][done[i]].second, Point());
				}
			}

			Gtk::Widget *ui_controls() {
				return nullptr;
			}

		private:
			World world;
			std::vector<std::pair<Point, Angle>> tasks[3];
			int time_steps;
			std::size_t done[3];
	};
}

HIGH_LEVEL_REGISTER(NCHL2013)

