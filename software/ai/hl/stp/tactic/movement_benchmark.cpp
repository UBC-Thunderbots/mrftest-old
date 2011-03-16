#include "ai/hl/stp/tactic/movement_benchmark.h"
#include "ai/hl/util.h"
#include <algorithm>

#include "geom/angle.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"
#include <ctime>
#include <glibmm.h>
#include <vector>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	
	const double pos_dis_threshold = 0.2;
	const double pos_vel_threshold = 0.2;
	const double ori_dis_threshold = 0.2;
	const double ori_vel_threshold = 0.2;

	const double PI = M_PI;
	
	const std::vector< std::pair<Point, double> > tasks = {
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(1, 0), PI),
		std::make_pair(Point(-2.5, 0), 0),
		std::make_pair(Point(2.5, 0), PI),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(-1, 0), PI / 2),
		std::make_pair(Point(0, 1), -PI / 2),
		std::make_pair(Point(0, -1), 0),
		std::make_pair(Point(2.5, 0), PI),
		std::make_pair(Point(-2.5, 0), -PI / 2),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(0.25, 0), PI / 2),
		std::make_pair(Point(0.1, 0.1), 0),
		std::make_pair(Point(-0.1, 0), PI),
		std::make_pair(Point(0, 0), PI),
		std::make_pair(Point(-0.25, -0.1), 0),
		std::make_pair(Point(0.25, 0.1), PI / 2),
		std::make_pair(Point(-0.1, 0), 0),
		std::make_pair(Point(0, 0), 0),
	};
	
	class MovementBenchmark : public Tactic {
		public:
			MovementBenchmark(const World &world) : Tactic(world,true) {
			}

		private:
			const Coordinate dest;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;			
			void execute();
			
			
			int time_steps;
			std::size_t finished;
			Point prev_pos;
			double prev_ori;
			std::time_t start_tasks, curr_tasks, end_tasks;
	};
	
	bool MovementBenchmark::done() const{
		return finished >= tasks.size();
	}

	Player::Ptr MovementBenchmark::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
	}

	void MovementBenchmark::execute() {
		
		if (finished == 0) {
			time_steps = 0;
			std::time(&start_tasks);
		} else if (finished > 0) {
			time_steps++;
		}
	
		const Point diff_pos = player->position() - tasks[finished].first;
		const Point vel_pos = player->position() - prev_pos;
		const double diff_ori = angle_mod(player->orientation() - tasks[finished].second);
		const double vel_ori = angle_mod(player->orientation() - prev_ori);

		if (diff_pos.len() < pos_dis_threshold && vel_pos.len() < pos_vel_threshold && fabs(diff_ori) < ori_dis_threshold && fabs(vel_ori) < ori_vel_threshold) {
			++finished;
		}

		std::time(&curr_tasks);

		if (finished >= tasks.size()) {
			// the first time that all tasks are done

			std::time(&end_tasks);
			double diff = difftime(end_tasks, start_tasks);

			LOG_INFO(Glib::ustring::compose("time steps taken: %1 time taken: %2", time_steps, diff));

			return;
		}

		prev_ori = player->orientation();
		prev_pos = player->position();
		player->move(tasks[finished].first, tasks[finished].second, 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::movement_benchmark(const World &world) {
	const Tactic::Ptr p(new MovementBenchmark(world));
	return p;
}

