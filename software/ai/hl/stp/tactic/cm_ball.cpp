#include "ai/hl/stp/tactic/cm_ball.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/evaluation/cm_evaluation.h"
#include "geom/angle.h"
#include "geom/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;
using AI::HL::STP::TCoordinate;
using AI::HL::STP::TRegion;
using AI::HL::STP::Coordinate;
using Evaluation::CMEvaluationPosition;

namespace {

	// TShoot
	// The amount we'd prefer to shoot at our previous angle.  If an another
	// at least this much bigger appears we'll switch to that.
	const double SHOOT_AIM_PREF_AMOUNT = 0.01745; // 1 degree
	// 0.1221  7 deg

	// We make sure the angle tolerance is always this big.
	const double SHOOT_MIN_ANGLE_TOLERANCE = 0.1745; // Pi / 16

	// Dribbles to open shot when nothing's open.
	const bool SHOOT_DRIBBLE_IF_NO_SHOT = true; 

	// TPass
	// The "width" of our teammate's front for the purpose of aiming a
	// pass.  Some connection to it's dribbler's width.
	const double PASS_TARGET_WIDTH = 0.030;

	class TShoot : public Tactic {
		public:
			enum Type { Aim, NoAim, Deflect};

			TShoot(const World &world) : Tactic(world, true) {}

		private:
			//Type type = Aim;
  			//int deflect_target = -1;

  			Point prev_target;
  			bool prev_target_set;

  			CMEvaluationPosition eval;

			bool kicked;
			bool done() const{
				return kicked;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}

			void execute();

			std::string description() const {
				return "tshoot";
			}
	};

	// this will require a steal move type in navigator (or steal skill/action) to work
	// probably just go to the ball to within a threshold distance, then start spinning to get the ball out of the enemy's dribbler
	/*
	class TSteal : public Tactic {
		public:
			TSteal(const World &world) : Tactic(world, true){}
			TSteal(const World &world, TCoordinate _target) : Tactic(world, true), target(_target) {}

		private:

			bool target_set;
  			
			TCoordinate target;		

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}

			void execute();

			std::string description() const {
				return "tsteal";
			}
	};
	*/

	class TClear : public Tactic {
		public:
			TClear(const World &world) : Tactic(world, true) {}

		private:
			Point prev_target;
  			bool prev_target_set;
			bool kicked;
			bool done() const{
				return kicked;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}

			void execute();

			std::string description() const {
				return "tclear";
			}
	};

	// No TSteal available yet
	/*
	class TActiveDef : public Tactic {
		public:
			TActiveDef(const World &world, TCoordinate _p1, TCoordinate _p2) : Tactic(world, true), p1(_p1), p2(_p2) {}

		private:
			TCoordinate p1, p2;
			bool intercepting;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(Point(0, 0)));
			}

			void execute();

			std::string description() const {
				return "tactive_def";
			}
	};
	*/
	class TPass : public Tactic {
		public:
			TPass(const World &world, const Coordinate _target) : Tactic(world, true), target(_target){}

		private:
			Coordinate target;
			bool kicked;
			bool done() const{ 
				return kicked; 
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}

			void execute();

			std::string description() const {
				return "tpass";
			}
	};

	class TReceivePass : public Tactic {
		public:
			TReceivePass(const World &world, const Coordinate _target) : Tactic(world, true), target(_target) {}

		private:
			Coordinate target;
			bool done() const{ 
				return player->has_ball(); 
			}			
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(target.position()));
			}

			void execute();

			std::string description() const {
				return "treceive_pass";
			}
	};

	class TDribbleToShoot : public Tactic {
		public:
			TDribbleToShoot(const World &world) : Tactic(world, true) {}

		private:

			bool kicked;
			bool done() const{ 
				return kicked; 
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}

			void execute();

			std::string description() const {
				return "tdribble_to_shoot";
			}
	};
	
	class TDribbleToRegion : public Tactic {
		public:
			TDribbleToRegion(const World &world, TRegion _region) : Tactic(world), region(_region){}

		private:
			TRegion region;
			
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}

			void execute();

			std::string description() const {
				return "tdribble_to_region";
			}
	};
	
	// uses steal, which is not implemented yet
	/*
	class TSpinToRegion : public Tactic {
		public:
			TSpinToRegion(const World &world, TRegion _region) : Tactic(world), region(_region){}

		private:
			TRegion region;
			
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}

			void execute();

			std::string description() const {
				return "tspin_to_region";
			}
	};
	// this seems useless =[
	class TReceiveDeflection : public Tactic {
		public:
			TReceiveDeflection(const World &world) : Tactic(world, true) {}

		private:
			bool got_to_spin;
			bool done() const{
				player->has_ball();
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(Point(0, 0)));
			}

			void execute();

			std::string description() const {
				return "treceive_deflection";
			}
	};
	*/
}


// not too sure how good is their shoot =/
void TShoot::execute() {
	kicked = false;
	Point ball = world.ball().position();

  	Point target;
  	double angle_tolerance;

  	if (!prev_target_set) {
    		prev_target = (ball - (player->position() - ball));
    		prev_target_set = true; 
  	}

  	//Type the_type = type;

  	// if (world.twoDefendersInTheirDZone())
    	//	the_type = NoAim;

  	bool got_target = false;

  	//if (the_type == Aim) {
    		got_target = Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), Point(world.field().length() / 2, -world.field().goal_width() / 2), Point(world.field().length() / 2, world.field().goal_width() / 2), OBS_EVERYTHING_BUT_US, prev_target, SHOOT_AIM_PREF_AMOUNT, target, angle_tolerance);
  	//}
	/*
  	if (the_type == Deflect) {
    		Point t = world.teammate_position(getTeammateId(deflect_target));
    		Point toward = t - ball;
    		Point toward_perp = toward.perp();

    		if (toward_perp.x < 0) towardperp *= -1;

    		got_target = Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), t + toward_perp.norm(0.080), t - toward_perp.norm(0.040), OBS_EVERYTHING_BUT_US, prev_target, SHOOT_AIM_PREF_AMOUNT, target, angle_tolerance);
    		got_target = true;

    		if ((target - ball).len() < 0.400)
      			target = (ball + target) / 2.0;
    		else 
      			target = target + (ball - target).norm(0.200);
  	}
	
  	if (the_type == NoAim || (!got_target && !SHOOT_DRIBBLE_IF_NO_SHOT)) {
    		// Guaranteed to return true and fill in the parameters when
    		// obs_flags is empty.
    		Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), world.their_goal_r, world.their_goal_l, 0, target, angle_tolerance);
    		got_target = true;
  	}
	*/
  	if (got_target) {
    
    		if (angle_tolerance < SHOOT_MIN_ANGLE_TOLERANCE) {
      			angle_tolerance = SHOOT_MIN_ANGLE_TOLERANCE;
    		}
    
    		prev_target = target;
    
    		// Aim for point in front of obstacles.
    		Point rtarget;
    		Point targ_ball;

    		targ_ball = target - ball;
		if (targ_ball.len()>0.5) targ_ball = targ_ball.norm(0.5);
    		Evaluation::obs_line_first(world, target-targ_ball*0.75, target, OBS_OPPONENTS, rtarget, Robot::MAX_RADIUS);

		Action::move(player, (rtarget - player->position()).orientation(), rtarget);
		kicked = Action::shoot(world, player, target);		
		
		/*
    		command.cmd = Robot::CmdMoveBall;
    		command.ball_target = target;
    		command.target = rtarget;
    		command.angle_tolerance = angle_tolerance;
    		command.ball_shot_type = Robot::BallShotOnGoal;
		*/
  	} else {
    		eval.update(world, 0);

		//Action::move(player, (target - player->position()).orientation(), target);
		//kicked = Action::shoot(world, player, target);

		kicked = Action::shoot(world, player, eval.point());
		Action::move(player, eval.angle(), eval.point());		

		/*
    		command.cmd = Robot::CmdDribble;
    		command.target = eval.point();
    		command.ball_target = eval.point();
    		command.angle = eval.angle();
		*/
  	}
}
/*
void TSteal::execute() {
	
}
*/
void TClear::execute() {
	kicked = false;
	Point ball = world.ball().position();

  	Point target;
  	double angle_tolerance;

  	bool aimed = false;

  	Point downfield[2];

  	//Robot::BallShotType shot_type= Robot::BallShotClear;

  	downfield[0] = Point(ball.x + 0.180, -world.field().width()/2);
  	downfield[1] = Point(ball.x + 0.180, world.field().width()/2);

  
  	if (!prev_target_set) prev_target = world.field().enemy_goal();

  	aimed = Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), downfield[0], downfield[1], OBS_EVERYTHING_BUT_US, prev_target, SHOOT_AIM_PREF_AMOUNT, target, angle_tolerance);

  	if (!aimed) {
    		// Guaranteed to return true and fill in the parameters when 
    		// obs_flags is empty.
    		Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), downfield[0], downfield[1], 0, target, angle_tolerance);
  	}

  	target = (target - ball).norm(std::min(world.field().length()/2 - ball.x, 1.000)) + ball;

  	// If the target tolerances include the goal then just aim there.
  	double a = (target - ball).orientation();
  	double a_to_goal = (world.field().enemy_goal() - ball).orientation();
  	//double a_to_goal_l = (world.their_goal_l - ball).orientation();
  	//double a_to_goal_r = (world.their_goal_r - ball).orientation();

  	if (std::fabs(angle_mod(a - a_to_goal)) < 0.8 * angle_tolerance) {
    		if (a > a_to_goal) {
      			target = world.field().enemy_goal();
      			angle_tolerance -= std::fabs(angle_mod(a - a_to_goal));
      			//shot_type = Robot::BallShotOnGoal;
    		} else if (a < a_to_goal) {
      			target = world.field().enemy_goal();
      			angle_tolerance -= std::fabs(angle_mod(a - a_to_goal));
      			//shot_type = Robot::BallShotOnGoal;
    		}
  	}

  	if (angle_tolerance < SHOOT_MIN_ANGLE_TOLERANCE) {
    		angle_tolerance = SHOOT_MIN_ANGLE_TOLERANCE;
  	}	

  	prev_target = target;
  	prev_target_set = true;

	kicked = Action::shoot(world, player, target);		
	Action::move(world, player, target);

	/*
  	command.cmd = Robot::CmdMoveBall;
  	command.target = target;
  	command.ball_target = target;
  	command.angle_tolerance = angle_tolerance;
  	command.ball_shot_type = shot_type;
	*/
}
/*
void TActiveDef::execute(){


}
*/
// might be better to just use our pass and receive pass
// these two tactics are implemented but not used in cm '02 =/

void TPass::execute(){
	kicked = false;
	Point p[2], targetp = target.position(), ball;
  	double angle_tolerance;

  	ball = world.ball().position();

  	targetp += (ball - targetp).norm(0.070);

  	p[0] = targetp + (targetp - ball).perp().norm(PASS_TARGET_WIDTH);
  	p[1] = targetp + (targetp - ball).perp().norm(PASS_TARGET_WIDTH);

  	Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), p[0], p[1], OBS_EVERYTHING_BUT_US, targetp, angle_tolerance);

  	// Set the drive target as 1m from the target, with some exceptions
  	// when close.
  	Point mytarget;

  	if ((targetp - ball).len() > 1.100)
    		mytarget = ball + (targetp - ball).norm(1.000);
  	else if ((targetp - ball).len() < 0.100)
    		mytarget = targetp;
  	else
    		mytarget = targetp + (ball - targetp).norm(0.100);

	Action::move(player, (targetp - player->position()).orientation(), targetp);
	kicked = Action::shoot(world, player, targetp);

	/*
  	command.cmd = Robot::CmdMoveBall;
  	command.target = targetp; // mytarget;
  	command.ball_target = targetp;
  	command.angle_tolerance = angle_tolerance;
  	command.ball_shot_type = Robot::BallShotPass;
	*/	
}

// should just use chase in our code?
// the passee seems to just orient towards the ball and don't move from its current position
// (I assume this is used after the position_for_pass tactic is used)

void TReceivePass::execute(){
	Action::move(world, player, target.position());
	Action::chase(world, player, world.ball().position());
	//command.cmd = Robot::CmdRecieveBall;
}

/*
// just use shoot action?
void TDribbleToShoot::execute(){


}
*/
// just use move dribble?
void TDribbleToRegion::execute(){
	Action::dribble(world, player, region.center(world));

}
/*
void TSpinToRegion::execute(){


}
*/

// this seems useless =[
// our chase action might be better
/*
void TReceiveDeflection::execute(){

	Point mypos = player->position();
	Point target = mypos;

  	command.cmd = Robot::CmdPosition;
  	command.target = target;
  	command.velocity = Point(0, 0);
  	command.angle = (target - mypos).orientation();
  	command.obs = OBS_EVERYTHING_BUT_ME(me) & ~OBS_BALL;

}
*/
Tactic::Ptr AI::HL::STP::Tactic::tshoot(const World &world) {
	Tactic::Ptr p(new TShoot(world));
	return p;
}
/*
Tactic::Ptr AI::HL::STP::Tactic::tsteal(const World &world) {
	Tactic::Ptr p(new TSteal(world));
	return p;
}
*/
Tactic::Ptr AI::HL::STP::Tactic::tclear(const World &world) {
	Tactic::Ptr p(new TClear(world));
	return p;
}
/*
Tactic::Ptr AI::HL::STP::Tactic::tactive_def(const World &world) {
    	// Tactic::Ptr p(new TActiveDef(world));
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (AI::HL::Util::posses_ball(world, enemy.get(i))) {
			return Tactic::Ptr p(new TSteal());
		}
	}

	return Tactic::Ptr p(new TClear());
}
*/

Tactic::Ptr AI::HL::STP::Tactic::tpass(const World &world, const Coordinate _target) {
    	Tactic::Ptr p(new TPass(world, _target));
    	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::treceive_pass(const World &world, const Coordinate _target) {
    	Tactic::Ptr p(new TReceivePass(world, _target));
    	return p;
}
/*
Tactic::Ptr AI::HL::STP::Tactic::tdribble_to_shoot(const World &world) {
    	Tactic::Ptr p(new TDribbleToShoot(world));
    	return p;
}
*/
Tactic::Ptr AI::HL::STP::Tactic::tdribble_to_region(const World &world, TRegion _region) {
    	Tactic::Ptr p(new TDribbleToRegion(world, _region));
    	return p;
}
/*
Tactic::Ptr AI::HL::STP::Tactic::tspin_to_region(const World &world, TRegion _region) {
    	Tactic::Ptr p(new TSpinToRegion(world, _region));
    	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::treceive_deflection(const World &world) {
    	Tactic::Ptr p(new TReceiveDeflection(world));
    	return p;
}
*/
