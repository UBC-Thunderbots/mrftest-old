#include "AI/Simulator.h"
#include "AI/AITeam.h"
#include "AI/CentralAnalyzingUnit.h"
#include "AI/RobotController.h"
#include "datapool/Vector2.h"
#include "datapool/World.h"

#include <unistd.h>

#define FRICTION_CONST 0.7

#define MAX_ACCELERATION 500.0
#define MAX_VELOCITY 750.0
#define BALL_ACCELERATION 577.0

Simulator::Simulator() {
	RobotController::setSimulation(true);

	PGoal goalW = Goal::create(Vector2(25, 200),  Vector2(25, 270),  Vector2(75, 217.5),  Vector2(75, 252.5),  16, Vector2(70, 235));
	PGoal goalE = Goal::create(Vector2(635, 200), Vector2(635, 270), Vector2(585, 217.5), Vector2(585, 252.5), 16, Vector2(590, 235));
	PField field = Field::create(660, 470, 25, 635, 25, 445, Vector2(330, 235), 50, goalW, goalE);

	PTeam friendlyTeam = AITeam::create(0);
	friendlyTeam->side(true);

	PTeam enemyTeam = AITeam::create(1);
	enemyTeam->side(false);

	World::init(friendlyTeam, enemyTeam, field);
	
	World &w = World::get();
	
	//Set the player properties:
	w.player(0)->position(Vector2(40, 235));
	w.player(0)->velocity(Vector2(0, 0));
	w.player(0)->acceleration(Vector2(0, 0));
	w.player(0)->radius(9);
	w.player(1)->position(Vector2(177, 130));
	w.player(1)->velocity(Vector2(0, 0));
	w.player(1)->acceleration(Vector2(0, 0));
	w.player(1)->radius(9);
	w.player(2)->position(Vector2(177, 340));
	w.player(2)->velocity(Vector2(0, 0));
	w.player(2)->acceleration(Vector2(0, 0));
	w.player(2)->radius(9);
	w.player(3)->position(Vector2(177, 235));
	w.player(3)->velocity(Vector2(0, 0));
	w.player(3)->acceleration(Vector2(0, 0));
	w.player(3)->radius(9);
	w.player(4)->position(Vector2(300, 235));
	w.player(4)->velocity(Vector2(0, 0));
	w.player(4)->acceleration(Vector2(0, 0));
	w.player(4)->radius(9);
	
	w.player(5)->position(Vector2(615, 235));
	w.player(5)->velocity(Vector2(0, 0));
	w.player(5)->acceleration(Vector2(0, 0));
	w.player(5)->radius(9);
	w.player(6)->position(Vector2(503, 130));
	w.player(6)->velocity(Vector2(0, 0));
	w.player(6)->acceleration(Vector2(0, 0));
	w.player(6)->radius(9);
	w.player(7)->position(Vector2(503, 340));
	w.player(7)->velocity(Vector2(0, 0));
	w.player(7)->acceleration(Vector2(0, 0));
	w.player(7)->radius(9);
	w.player(8)->position(Vector2(503, 235));
	w.player(8)->velocity(Vector2(0, 0));
	w.player(8)->acceleration(Vector2(0, 0));
	w.player(8)->radius(9);
	w.player(9)->position(Vector2(400, 235));
	w.player(9)->velocity(Vector2(0, 0));
	w.player(9)->acceleration(Vector2(0, 0));
	w.player(9)->radius(9);
	
	//Set the ball properties:
	w.ball()->position(Vector2(330, 235));
	w.ball()->velocity(Vector2(0, 0));
	w.ball()->acceleration(Vector2(0, 0));
	w.ball()->radius(2.15);
}

void Simulator::update() {	
	usleep(10000);

	World &world = World::get();
	PField field = world.field();

	for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
		PPlayer player = world.player(i);

		Vector2 pos = player->position();
		Vector2 vel = player->velocity();
		Vector2 acc = player->acceleration();
		
		double maxacc = field->convertMmToCoord(MAX_ACCELERATION) / (CentralAnalyzingUnit::FRAMES_PER_SECOND * CentralAnalyzingUnit::FRAMES_PER_SECOND);
		double maxvel = field->convertMmToCoord(MAX_VELOCITY) / CentralAnalyzingUnit::FRAMES_PER_SECOND;
		/*if (acc.length() > maxacc){
			acc = acc / acc.length() * maxacc;
			player->acceleration(acc);	
		}*/

		pos += vel/2.0;
		vel += acc;
		
		
		if (vel.length() > maxvel)
			vel = vel / vel.length() * maxvel;
	
		vel *= FRICTION_CONST;
		pos += vel/2.0;
		player->position(pos);
		player->velocity(vel);
		
		// Make sure they are not overlapping each other:
		PPlayer closest = CentralAnalyzingUnit::closestRobot(player, CentralAnalyzingUnit::TEAM_ANY, true);
		
		Vector2 diff = pos - closest->position();
		
		while (diff.length() < 2.0 * player->radius()) {
			diff *= 1.0 / diff.length();
			diff *= 2.0 * player->radius();
			diff += closest->position();
			player->position(diff);
		}
	}

	PBall ball = world.ball();
	Vector2 pos = ball->position();
	Vector2 vel = ball->velocity();
	Vector2 acc = ball->acceleration();

	if (vel.length() < 1E-9) acc = Vector2(0,0);
	else{
		acc = vel / (-vel.length());
		acc *= field->convertMmToCoord(BALL_ACCELERATION) / (CentralAnalyzingUnit::FRAMES_PER_SECOND * CentralAnalyzingUnit::FRAMES_PER_SECOND);
	}
	pos += vel/2.0;
	vel += acc;
	//pos += vel;
	
	//vel *= 0.98; // The ball has much less resistance than the robots.
	pos += vel/2.0;

	// Make sure the ball does not go out of bounds:
	if (pos.x < field->west()) {	
		if (pos.y > field->westGoal()->north.y &&
			pos.y < field->westGoal()->south.y) {
			if (world.friendlyTeam()->side())
				world.enemyTeam()->score(world.enemyTeam()->score() + 1);
			else
				world.friendlyTeam()->score(world.friendlyTeam()->score() + 1);
		}
		world.playType(PlayType::start);
		pos = Vector2(field->centerCircle());
		vel = Vector2(0, 0);
		acc = Vector2(0, 0);
		for (unsigned int i = 0; i < 10; i++)
			if (world.player(i)->hasBall())
				world.player(i)->hasBall(false);
	} else if (pos.y < field->north()) {
		world.playType(PlayType::start);
		pos = Vector2(field->centerCircle());
		vel = Vector2(0, 0);
		acc = Vector2(0, 0);
		for (unsigned int i = 0; i < 10; i++)
			if (world.player(i)->hasBall())
				world.player(i)->hasBall(false);
	} else if (pos.x > field->east()) {
		if (pos.y > field->eastGoal()->north.y &&
			pos.y < field->eastGoal()->south.y) {
			if (world.friendlyTeam()->side())
				world.friendlyTeam()->score(world.friendlyTeam()->score() + 1);
			else
				world.enemyTeam()->score(world.enemyTeam()->score() + 1);
		}
		world.playType(PlayType::start);
		pos = Vector2(field->centerCircle());
		vel = Vector2(0, 0);
		acc = Vector2(0, 0);
		for (unsigned int i = 0; i < 10; i++)
			if (world.player(i)->hasBall())
				world.player(i)->hasBall(false);
	} else if (pos.y > field->south()) {
		world.playType(PlayType::start);
		pos = Vector2(field->centerCircle());
		vel = Vector2(0, 0);
		acc = Vector2(0, 0);
		for (unsigned int i = 0; i < 10; i++)
			if (world.player(i)->hasBall())
				world.player(i)->hasBall(false);
	}

	switch(world.playType()){
		case PlayType::preparePenaltyKick:
			if (world.team(0)->specialPossession())
				pos = Vector2(field->eastGoal()->penalty);
			else pos = Vector2(field->westGoal()->penalty);
			vel = Vector2(0,0);
			acc = Vector2(0,0);
			break;
	}
	
	ball->acceleration(acc);
	ball->position(pos);
	ball->velocity(vel);			
	
	// Keep track if there are multiple robots fighting over the ball
	// with one (first robot) and two (second robot).
	PPlayer one, two;
	for (unsigned int i = 0; i < 10; i++) {
		PPlayer cur = world.player(i);
		Vector2 diff = ball->position() - cur->position();
		if (diff.length() <= cur->radius() + ball->radius()) {
			if (!one)      one = cur;
			else if (!two) two = cur;
		}
		cur->hasBall(false);
	}

	if (one) {
		PPlayer winner = one;
		// Fight for possession of the ball!
		if (two && (rand() % 2))
			winner = two;
		if (World::get().playType() != PlayType::stop && World::get().playType() != PlayType::prepareKickoff 
                   && World::get().playType() != PlayType::preparePenaltyKick){
			winner->hasBall(true);
			// Set the ball position in front of the robot with the ball.
			double direction = winner->orientation();	
			Vector2 ballPos = direction;
			ballPos *= winner->radius();
			ball->position(winner->position() + ballPos);
			ball->velocity(winner->velocity());
		}
	}

}

