#include "ai/hl/strategy.h"
#include "uicomponents/param.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

#include <vector>
#include <ctime>
#include <glibmm.h>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

#define TUNE_HALF

namespace {

	const double pos_dis_threshold = 0.2;
	const double pos_vel_threshold = 0.2;
	const double ori_dis_threshold = 0.2;
	const double ori_vel_threshold = 0.2;
	
	const double PI = M_PI;

#ifdef TUNE_HALF
	const std::pair<Point, double> default_tasks[] =
	{
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(0.5, 0), PI),
		std::make_pair(Point(2.5, 0), 0),
		std::make_pair(Point(0.5, 1.2), PI),
		std::make_pair(Point(1, -0.6), 0),
		/*std::make_pair(Point(2, 0.6), PI/2),
		std::make_pair(Point(1, -0.6), -PI/2),
		std::make_pair(Point(0.5, 0), 0),
		std::make_pair(Point(2.5, 0.6), -PI/2),
		std::make_pair(Point(1.2, 0), 0),*/
	};
#endif

#ifdef TUNE_FULL
	const std::pair<Point, double> default_tasks[] =
	{
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(1, 0), PI),
		std::make_pair(Point(-2.5, 0), 0),
		std::make_pair(Point(2.5, 0), PI),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(-1, 0), PI/2),
		std::make_pair(Point(0, 1), -PI/2),
		std::make_pair(Point(0, -1), 0),
		std::make_pair(Point(2.5, 0), PI),
		std::make_pair(Point(-2.5, 0), -PI/2),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(0.25, 0), PI/2),
		std::make_pair(Point(0.1, 0.1), 0),
		std::make_pair(Point(-0.1, 0), PI),
		std::make_pair(Point(0, 0), PI),
		std::make_pair(Point(-0.25, -0.1), 0),
		std::make_pair(Point(0.25, 0.1), PI/2),
		std::make_pair(Point(-0.1, 0), 0),
		std::make_pair(Point(0, 0), 0),
	};
#endif

#ifdef NO_TUNE_ROTATION
	const std::pair<Point, double> default_tasks[] =
	{
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(1, 0), 0),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(-1, 0), 0),
		std::make_pair(Point(0, 1), 0),
		std::make_pair(Point(0, -1), 0),
		std::make_pair(Point(2.5, 0), 0),
		std::make_pair(Point(-2.5, 0), 0),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(0.25, 0), 0),
		std::make_pair(Point(0.1, 0.1), 0),
		std::make_pair(Point(-0.1, 0), 0),
		std::make_pair(Point(0, 0), 0),
		std::make_pair(Point(-0.25, -0.1), 0),
		std::make_pair(Point(0.25, 0.1), 0),
		std::make_pair(Point(-0.1, 0), 0),
		std::make_pair(Point(0, 0), 0),
	};
#endif

	const int default_tasks_n = sizeof(default_tasks) / sizeof(default_tasks[0]);

	/**
	 * Manages the robots during a stoppage in place (that is, when the game is in PlayType::STOP).
	 */
	class MovementBenchmarkStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * Creates a new MovementBenchmarkStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			MovementBenchmarkStrategy(World &world);
			~MovementBenchmarkStrategy();

			void play();
			void reset();

			std::vector<std::pair<Point, double> > tasks;
			int time_steps;
			size_t done;
			Point prev_pos;
			double prev_ori;
			time_t start_tasks, curr_tasks, end_tasks;
	};

	/**
	 * A factory for constructing \ref MovementBenchmarkStrategy "StopStrategies".
	 */
	class MovementBenchmarkStrategyFactory : public StrategyFactory {
		public:
			MovementBenchmarkStrategyFactory();
			~MovementBenchmarkStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of MovementBenchmarkStrategyFactory.
	 */
	MovementBenchmarkStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
	};

	StrategyFactory &MovementBenchmarkStrategy::factory() const {
		return factory_instance;
	}

	void MovementBenchmarkStrategy::reset() {
		done = 0;
		time_steps = 0;
	}

	void MovementBenchmarkStrategy::play() {
		FriendlyTeam &friendly = world.friendly_team();
		if (friendly.size() != 1) {
			//std::cerr << "error: must have only 1 robot in the team!" << std::endl;
			return;
		}
		if (done >= tasks.size()) return;
		if (done == 0) {
			time_steps = 0;
			time(&start_tasks);
		} else if (done > 0) {
			time_steps++;
		}

		Player::Ptr runner = friendly.get(0);

		const Point diff_pos = runner->position() - tasks[done].first;
		const Point vel_pos = runner->position() - prev_pos;
		const double diff_ori = angle_mod(runner->orientation() - tasks[done].second);
		const double vel_ori = angle_mod(runner->orientation() - prev_ori);

		if (diff_pos.len() < pos_dis_threshold && vel_pos.len() < pos_vel_threshold && fabs(diff_ori) < ori_dis_threshold && fabs(vel_ori) < ori_vel_threshold) {
			++done;
		}

		time(&curr_tasks);

		if (done >= tasks.size()) {
			// the first time that all tasks are done

			time(&end_tasks);
			double diff = difftime(end_tasks, start_tasks);

			LOG_INFO(Glib::ustring::compose("time steps taken: %1 time taken: %2", time_steps, diff));

			return;
		}

		prev_ori = runner->orientation();
		prev_pos = runner->position();
		runner->move(tasks[done].first, tasks[done].second, 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
	}

	Strategy::Ptr MovementBenchmarkStrategy::create(World &world) {
		const Strategy::Ptr p(new MovementBenchmarkStrategy(world));
		return p;
	}

	MovementBenchmarkStrategy::MovementBenchmarkStrategy(World &world) : Strategy(world), tasks(default_tasks, default_tasks + default_tasks_n), done(0), prev_pos(0.0, 0.0), prev_ori(0.0) {
		time_steps = 0;
		done = tasks.size();
	}

	MovementBenchmarkStrategy::~MovementBenchmarkStrategy() {
	}

	MovementBenchmarkStrategyFactory::MovementBenchmarkStrategyFactory() : StrategyFactory("Movement Benchmark", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	MovementBenchmarkStrategyFactory::~MovementBenchmarkStrategyFactory() {
	}

	Strategy::Ptr MovementBenchmarkStrategyFactory::create_strategy(World &world) const {
		return MovementBenchmarkStrategy::create(world);
	}
}

