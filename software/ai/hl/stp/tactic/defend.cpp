#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/old/tactics.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Evaluation::Defense;

namespace {
	/**
	 * A generic defend tactic for all 3 types.
	 */
	class Defend : public Tactic {
		public:
			Defend(const World &world, const Defense &defense) : Tactic(world), defense(defense) {
			}

		protected:
			const Defense &defense;
			virtual Point dest() const = 0;

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	Player::Ptr Defend::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
	}

	void Defend::execute() {
		// TODO: do more interesting stuff
		player->move(dest(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, param.move_priority);
	}

	/**
	 * Goalie.
	 */
	class Goalie : public Defend {
		public:
			Goalie(const World &world, const Defense &defense) : Defend(world, defense) {
			}

		private:
			Point dest() const;
	};

	Point Goalie::dest() const {
		return defense.goalie_dest();
	}

	/**
	 * Defender1.
	 */
	class Defender1 : public Defend {
		public:
			Defender1(const World &world, const Defense &defense) : Defend(world, defense) {
			}

		private:
			Point dest() const;
	};

	Point Defender1::dest() const {
		return defense.defender1_dest();
	}

	/**
	 * Defender2.
	 */
	class Defender2 : public Defend {
		public:
			Defender2(const World &world, const Defense &defense) : Defend(world, defense) {
			}

		private:
			Point dest() const;
	};

	Point Defender2::dest() const {
		return defense.defender2_dest();
	}
}

Tactic::Ptr AI::HL::STP::Tactic::goalie(World &world, const Defense &defense) {
	const Tactic::Ptr p(new Goalie(world, defense));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defender1(World &world, const Defense &defense) {
	const Tactic::Ptr p(new Defender1(world, defense));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defender2(World &world, const Defense &defense) {
	const Tactic::Ptr p(new Defender2(world, defense));
	return p;
}

