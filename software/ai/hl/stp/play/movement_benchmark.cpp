#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/movement_benchmark.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	/**
	 * Condition:
	 * - ball not under any possesion
	 * - at least 1 players
	 *
	 * Objective:
	 * - movement benchmark
	 */
	class MovementBenchmark : public Play {
		public:
			MovementBenchmark(const World &world);
			~MovementBenchmark();

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<MovementBenchmark> factory_instance("Movement Benchmark");

	const PlayFactory &MovementBenchmark::factory() const {
		return factory_instance;
	}

	MovementBenchmark::MovementBenchmark(const World &world) : Play(world) {
	}

	MovementBenchmark::~MovementBenchmark() {
	}

	bool MovementBenchmark::invariant() const {
		return Predicates::playtype(world, PlayType::PLAY) && Predicates::our_team_size_at_least(world, 1);
	}

	bool MovementBenchmark::applicable() const {
		return false;
		//return Predicates::none_ball(world);
	}

	bool MovementBenchmark::done() const {
		return Predicates::playtype(world, PlayType::STOP);
	}

	bool MovementBenchmark::fail() const {
		return false;
	}

	void MovementBenchmark::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// GOALIE
		// movement benchmark
		goalie_role.push_back(movement_benchmark(world));

		// ROLE 1
		// obstacle
		roles[0].push_back(move(world,Point(0.5, 0)));
		
		// ROLE 2
		// obstacle
		roles[1].push_back(move(world,Point(-0.5, 0)));

		// ROLE 3 (optional)
		// obstacle
		roles[2].push_back(move(world,Point(0, 0.6)));

		// ROLE 4 (optional)
		// obstacle
		roles[3].push_back(move(world,Point(0, -0.6)));
	}
}

