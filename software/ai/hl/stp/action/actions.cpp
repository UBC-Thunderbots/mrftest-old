#include "ai/flags.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam lone_goalie_dist("Lone goalie distance to goal post (m)", 0.30, 0.05, 1.0);
	DoubleParam ball_dangerous_speed("defensive2: threatening ball speed", 0.1, 0.1, 10.0); 

}

void AI::HL::STP::Action::chase(const World &world, Player::Ptr player, const unsigned int flags) {
	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
}

void AI::HL::STP::Action::repel(const World &world, Player::Ptr player, const unsigned int flags) {
	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);
		return;
	}

	// just shoot as long as it's not in backwards direction
	if (player->orientation() < M_PI / 2 && player->orientation() > -M_PI / 2) {
		if (player->chicker_ready_time() == 0) {
			player->kick(10.0);
		}
	}

	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);

	// all enemies are obstacles
	/*
	   std::vector<Point> obstacles;
	   EnemyTeam &enemy = world.enemy_team();
	   for (std::size_t i = 0; i < enemy.size(); ++i) {
	   obstacles.push_back(enemy.get(i)->position());
	   }

	   const Field &f = world.field();

	   // vertical line at the enemy goal area
	   // basically u want the ball to be somewhere there
	   const Point p1 = Point(f.length() / 2.0, -f.width() / 2.0);
	   const Point p2 = Point(f.length() / 2.0, f.width() / 2.0);
	   std::pair<Point, double> target = angle_sweep_circles(player->position(), p1, p2, obstacles, Robot::MAX_RADIUS);

	   AI::HL::STP::Action::shoot(world, player, flags, target.first);
	 */
}

void AI::HL::STP::Action::free_move(const World &world, Player::Ptr player, const Point p) {
	player->move(p, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
}

void AI::HL::STP::Action::lone_goalie(const World &world, Player::Ptr player) {
	// if ball is inside the defense area, must repel!
	if (AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position())) {
		repel(world, player, 0);
		return;
	}

	// Check if ball is threatening to our goal
	Point ballvel = world.ball().velocity();
	Point ballpos = world.ball().position();
	Point rushpos, goalpos;
	if (ballvel.len() > ball_dangerous_speed && ballvel.x < -1e-6){
		rushpos = line_intersect(ballpos, ballpos + ballvel, 
					Point(-world.field().length()/2.0 + 1.5*Robot::MAX_RADIUS, 1.0),
					Point(-world.field().length()/2.0 + 1.5*Robot::MAX_RADIUS, -1.0));

		goalpos = line_intersect(ballpos, ballpos + ballvel, 
					Point(world.field().length()/2.0, 1.0),
					Point(world.field().length()/2.0, -1.0));
		LOG_INFO(Glib::ustring::compose("ball heading towards our side of the field: rushpos.y = %1, goalpos.y = %2", rushpos.y, goalpos.y));
		
	} 
	
	// if ball is coming in at dangerous velocity, must rush!
	if (ballvel.len() > ball_dangerous_speed && ballvel.x < -1e-6 && (std::min(std::fabs(goalpos.y),std::fabs(rushpos.y)) < world.field().goal_width()/2.0)){
	
		rushpos.y = std::min(rushpos.y, world.field().goal_width()/2.0);
		rushpos.y = std::max(rushpos.y, world.field().goal_width()/2.0);
		player->move(rushpos, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);
		
	} else {
		// Patrol
		const Point default_pos = Point(-0.45 * world.field().length(), 0);
		const Point centre_of_goal = world.field().friendly_goal();
		Point target = world.ball().position() - centre_of_goal;
		target = target * (lone_goalie_dist / target.len());
		target += centre_of_goal;
		player->move(target, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}
}

void AI::HL::STP::Action::block(const World &world, Player::Ptr player, const unsigned int flags, Robot::Ptr robot) {
	
	// should have threshold distance, half a robot radius?
	
	//Point near_enemy(enemy->evaluate()->position().x - Robot::MAX_RADIUS * 3, enemy->evaluate()->position().y);
	//player->move(near_enemy, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);

	Point dirToGoal = (world.field().friendly_goal() - robot->position()).norm();
	player->move(robot->position() + (0.5*Robot::MAX_RADIUS*dirToGoal), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
}

void AI::HL::STP::Action::block_pass(const World &world, Player::Ptr player, const unsigned int flags, Robot::Ptr robot) {

	// should have threshold distance, half a robot radius?
	// TODO: Use this somehow
	Point dirToBall = (world.ball().position() - robot->position()).norm();
	player->move(robot->position() + (0.5*Robot::MAX_RADIUS*dirToBall), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
}

