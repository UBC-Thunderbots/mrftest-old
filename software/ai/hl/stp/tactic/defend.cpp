#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;

namespace {
	/**
	 * Goalie in a team of N robots.
	 */
	class Goalie : public Tactic {
		public:
			Goalie(const World &world) : Tactic(world) {
			}

		private:
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
			std::string description() const {
				return "goalie (helped by defender)";
			}
	};

	/**
	 * Primary defender.
	 */
	class Primary : public Tactic {
		public:
			Primary(const World &world) : Tactic(world) {
			}
		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "defender (helps goalie)";
			}
	};

	/**
	 * Secondary defender.
	 */
	class Secondary : public Tactic {
		public:
			Secondary(const World &world) : Tactic(world) {
			}
		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "extra defender";
			}
	};
	
	/**
	 * STP Tactic DefendLine 
 	 */
	class TDefendLine : public Tactic {
		
		private:
	  		// TCoordinate p[2];
			Point p[2];
			double distmin, distmax;
			  
			bool intercepting;

		public:
	  		// TDefendLine(TCoordinate p1, TCoordinate p2, double _distmin, double _distmax);
			TDefendLine(Point p1, Point p2, double _distmin, double _distmax);			

	  		// static Tactic *parser(const char *param_string);
	  		// virtual Tactic *clone() const { return new TDefendLine(*this); }

	  		// virtual void command(World &world, int me, Robot::RobotCommand &command, bool debug);

	  		// virtual Status isDone(World &world, int me) { return intercepting ? Busy : the_status; }
			
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "defend_line";
			}
	};

	class TDefendPoint : public Tactic {
		
		private:
	  		// TCoordinate center;
			Point center;
	  		double distmin, distmax;

	  		bool intercepting;

		public:
	  		// TDefendPoint(TCoordinate _center, double _distmin, double _distmax);
			TDefendPoint(Point _center, double _distmin, double _distmax);			

	  		// static Tactic *parser(const char *param_string);
	  		// virtual Tactic *clone() const { return new TDefendPoint(*this); }
	
	  		// virtual void command(World &world, int me, Robot::RobotCommand &command, bool debug);

	  		// virtual Status isDone(World &world, int me) { return intercepting ? Busy : the_status; }

			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "defend_point";
			}
	};

	class TDefendLane : public Tactic {
		
		private:
	  		// TCoordinate p[2];
			Point p[2];
	  		bool intercepting;

		public:
	  		// TDefendLane(TCoordinate _p1, TCoordinate _p2);
			TDefendLane(Point _p1, Point _p2);

	  		// static Tactic *parser(const char *param_string);
	  		// virtual Tactic *clone() const { return new TDefendLane(*this); }

	  		// virtual void command(World &world, int me, Robot::RobotCommand &command, bool debug);

	  		// virtual Status isDone(World &world, int me) { return intercepting ? Busy : the_status; }
			
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "defend_lane";
			}
	};



	void Goalie::execute() {
		auto waypoints = Evaluation::evaluate_defense(world);
		//player->move(waypoints[0], (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::HIGH);
		Action::goalie_move(world, player,  waypoints[0]);
	}

	Player::Ptr Primary::select(const std::set<Player::Ptr> &players) const {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[1];
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
	}

	void Primary::execute() {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[1];
		// TODO: medium priority for D = 1, low for D = 2
		Action::move(world, player, dest);
	}

	Player::Ptr Secondary::select(const std::set<Player::Ptr> &players) const {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[2];
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
	}

	void Secondary::execute() {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[2];
		Action::move(world, player, dest);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_goalie(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Goalie(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_defender(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Primary(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Secondary(world));
	return p;
}

