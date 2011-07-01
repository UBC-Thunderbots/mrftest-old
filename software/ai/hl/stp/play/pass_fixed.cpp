#include "ai/hl/util.h"
#include "ai/hl/world.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/region.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/tactic/pass.h"

#include <sstream>

using namespace AI::HL::W;
using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Predicates;
using namespace AI::HL::STP::Tactic;
namespace Predicates = AI::HL::STP::Predicates;

namespace {

	DoubleParam min_dist("Minimum distance for pass play", "STP/Pass", 1.0, 0.0, 5.0);

	class PassPlay : public Play {
		public:
			PassPlay(const World& world, const PlayFactory& factory, const Point target) : Play(world), _factory(factory), target(target) {
			}

			const PlayFactory &factory() const {
				return _factory;
			}

			bool invariant() const {
				return Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3) && !Predicates::fight_ball(world);
			}

			bool done() const {
				return Predicates::baller_can_shoot(world);
			}

			bool applicable() const {
				return Predicates::our_ball(world) && Predicates::baller_can_shoot_target(world, target, true)
					&& (world.ball().position() - target).len() > min_dist && !Predicates::baller_can_shoot(world);
			}

			bool fail() const {
				return Predicates::their_ball(world);
			}

			void assign(std::vector<Tactic::Tactic::Ptr> &goalie_role, std::vector<Tactic::Tactic::Ptr>(&roles)[4]) {
				// GOALIE
				goalie_role.push_back(goalie_dynamic(world, 2));
				// ROLE 1
				// passer
				roles[0].push_back(passer_shoot_target(world, target));
				// ROLE 2
				// passee
				roles[1].push_back(passee_move_target(world, target));
				// ROLE 3
				// defend
				roles[2].push_back(defend_duo_defender(world));
				// ROLE 4
				// offensive support through blocking closest enemy to ball
				roles[3].push_back(offend(world));

				/////////////////////////////////////
				// 2nd set of tactics 
				/////////////////////////////////////

				// GOALIE
				goalie_role.push_back(goalie_dynamic(world, 1));
				// ROLE 1
				// passer
				roles[0].push_back(passee_receive_target(world, target));
				// ROLE 2
				// passee
				roles[1].push_back(defend_duo_defender(world));
				// ROLE 3
				// defend
				roles[2].push_back(offend(world));
				// ROLE 4
				// offensive support through blocking closest enemy to ball
				roles[3].push_back(offend_secondary(world));
			}

			const PlayFactory& _factory;
			Point target;
	};

	class PassPlayFactoryImpl : public PlayFactory {
		public:
			PassPlayFactoryImpl(const char *name, const Point target) : PlayFactory(name), target(target) {
			}
			Play::Ptr create(const World &world) const {
				const Play::Ptr p(new PassPlay(world, *this, target));
				return p;
			}
			Point target;
	};

	/**
	 * A Pass Play factory that only concerns itself with passing to a specific location.
	 */
	class PassPlayFactory {
		public:
			PassPlayFactory(const Point target);

			~PassPlayFactory();

		protected:
			PlayFactory* instance;
			std::string name;
	};

	PassPlayFactory::PassPlayFactory(const Point target) {
		std::ostringstream ssn;
		ssn << "Pass Play " << target.x << " " << target.y;
		name = ssn.str();
		instance = new PassPlayFactoryImpl(name.c_str(), target);
	}

	PassPlayFactory::~PassPlayFactory() {
		if (!instance) {
			delete instance;
		}
	}

	PassPlayFactory p1(Point(0, 0));
	PassPlayFactory p2(Point(1.2, 1.5));
	PassPlayFactory p3(Point(1.2, -1.5));
}

