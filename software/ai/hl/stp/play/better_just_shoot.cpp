#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
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
	 * - at least 1 player (Can be just the Goalie)
	 * - ball under team possesion
	 *
	 * Objective:
	 * - Attempt to shoot ball regardless of how many players we have! (Max 4) 
	 */
	class BetterJustShoot : public Play {
		public:
			BetterJustShoot(const AI::HL::W::World &world);
			~BetterJustShoot();

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<BetterJustShoot> factory_instance("Better Just Shoot");

	const PlayFactory &BetterJustShoot::factory() const {
		return factory_instance;
	}

	BetterJustShoot::BetterJustShoot(const World &world) : Play(world) {
	}

	BetterJustShoot::~BetterJustShoot() {
	}

	bool BetterJustShoot::invariant() const {
		return Predicates::playtype(world, PlayType::PLAY) && Predicates::our_team_size_at_least(world, 1);
	}

	bool BetterJustShoot::applicable() const {
		return Predicates::our_ball(world) && Predicates::baller_can_shoot(world);
	}

	bool BetterJustShoot::done() const {
		return Predicates::goal(world);
	}

	bool BetterJustShoot::fail() const {
		return Predicates::their_ball(world);
	}

	void BetterJustShoot::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		//Case 0 - Goalie 
			if(world.friendly_team().size() == 1){
				// GOALIE
				goalie_role.push_back(shoot(world));
				}

		//Case 1 - Goalie + 1 Player
			if(world.friendly_team().size() == 2){
				// GOALIE
				goalie_role.push_back(defend_duo_goalie(world));
				// ROLE 1
				// shoot
				roles[0].push_back(shoot(world));
				}


		//Case 2 - Goalie + 2 players
			if(world.friendly_team().size() == 3){
				// GOALIE
				goalie_role.push_back(defend_duo_goalie(world));
				// ROLE 1
				// shoot
				roles[0].push_back(shoot(world));

				// ROLE 2
				// defend
				roles[1].push_back(defend_duo_defender(world));
				}

		//Case 3 - Goalie + 3 players
			if(world.friendly_team().size() == 4){
				// GOALIE
				goalie_role.push_back(defend_duo_goalie(world));
				// ROLE 1
				// shoot
				roles[0].push_back(shoot(world));

				// ROLE 2
				// defend
				roles[1].push_back(defend_duo_defender(world));

				// ROLE 3 (optional)
				// offensive support
				roles[2].push_back(offend(world));
				}

		//Case 4 - Goalie + 4 players
			if(world.friendly_team().size() == 5){
				// GOALIE
				goalie_role.push_back(defend_duo_goalie(world));
				// ROLE 1
				// shoot
				roles[0].push_back(shoot(world));

				// ROLE 2
				// defend
				roles[1].push_back(defend_duo_defender(world));

				// ROLE 3 (optional)
				// offensive support
				roles[2].push_back(offend(world));

				// ROLE 4 (optional)
				// offensive support
				roles[3].push_back(offend(world));
				}
	}
}


