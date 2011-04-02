#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	/**
	 * Condition:
	 * - ball under team possesion
	 * - ball in their corner
	 * - have at least 3 players (one goalie, one defender, one shooter)
	 *
	 * Objective:
	 * - shoot the ball to enemy goal while passing the ball between the passer and passee
	 */
	class TheirCornerPlay : public Play {
		public:
			TheirCornerPlay(const AI::HL::W::World &world);
			~TheirCornerPlay();

		private:
			bool invariant() const; 
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<TheirCornerPlay> factory_instance("Their Corner Play");

	const PlayFactory &TheirCornerPlay::factory() const {
		return factory_instance;
	}

	TheirCornerPlay::TheirCornerPlay(const World &world) : Play(world) {
	}

	TheirCornerPlay::~TheirCornerPlay() {
	}
	
	bool TheirCornerPlay::invariant() const {
		return Predicates::playtype(world, PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3);
	}

	bool TheirCornerPlay::applicable() const {
		return Predicates::our_ball(world) && Predicates::ball_in_their_corner(world) && !Predicates::baller_can_shoot(world);
	}

	bool TheirCornerPlay::done() const {
		return !Predicates::ball_in_their_corner(world);
	}

	bool TheirCornerPlay::fail() const {
		return Predicates::their_ball(world);
	}

	void TheirCornerPlay::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);
		// const FriendlyTeam &friendly = world.friendly_team();

		// GOALIE
		goalie_role.push_back(defend_duo_goalie(world));
		
		// ROLE 1
		// shoot towards the midfield
		roles[0].push_back(shoot(world, Point(-world.ball().position().x, -world.ball().position().y)));

		// ROLE 2
		// defend
		roles[1].push_back(defend_duo_defender(world));

		// ROLE 3 (optional)
		// defend 
		roles[2].push_back(defend_duo_extra(world));

		// ROLE 4 (optional)
		// offensive support
		roles[3].push_back(offend(world));
	}
}

