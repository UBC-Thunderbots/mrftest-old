#include "AI/Simulator.h"
#include "AI/CentralAnalyzingUnit.h"
#include "AI/RobotController.h"
#include "datapool/Vector2.h"
#include "datapool/World.h"

#include <unistd.h>

#define FRICTION_CONST 0.7

#define MAX_ACCELERATION 2000.0
#define MAX_VELOCITY 2000.0
#define BALL_ACCELERATION 577.0

Simulator::Simulator(Team &friendly, Team &enemy) {
	RobotController::setSimulation(true);

	Field field;
	field.width(660);
	field.height(470);
	field.west(25);
	field.east(635);
	field.north(25);
	field.south(445);
	field.centerCircle(Vector2(330, 235));
	field.centerCircleRadius(50);
	field.infinity(1e9);

	field.westGoal().north.x = 25;
	field.westGoal().north.y = 200;
	field.westGoal().south.x = 25;
	field.westGoal().south.y = 270;
	field.westGoal().defenseN.x = 75;
	field.westGoal().defenseN.y = 217.5;
	field.westGoal().defenseS.x = 75;
	field.westGoal().defenseS.y = 252.5;
	field.westGoal().height = 16;
	field.westGoal().penalty.x = 70;
	field.westGoal().penalty.y = 235;

	field.eastGoal().north.x = 635;
	field.eastGoal().north.y = 200;
	field.eastGoal().south.x = 635;
	field.eastGoal().south.y = 270;
	field.eastGoal().defenseN.x = 585;
	field.eastGoal().defenseN.y = 217.5;
	field.eastGoal().defenseS.x = 585;
	field.eastGoal().defenseS.y = 252.5;
	field.eastGoal().height = 16;
	field.eastGoal().penalty.x = 590;
	field.eastGoal().penalty.y = 235;

	World::init(friendly, enemy, field);
	
	World &w = World::get();
	
	//Set the player properties:
	w.player(0)->position(Vector2(40, 235));
	w.player(0)->simulatedVelocity(Vector2(0, 0));
	w.player(0)->simulatedAcceleration(Vector2(0, 0));
	w.player(0)->radius(9);
	w.player(1)->position(Vector2(177, 130));
	w.player(1)->simulatedVelocity(Vector2(0, 0));
	w.player(1)->simulatedAcceleration(Vector2(0, 0));
	w.player(1)->radius(9);
	w.player(2)->position(Vector2(177, 340));
	w.player(2)->simulatedVelocity(Vector2(0, 0));
	w.player(2)->simulatedAcceleration(Vector2(0, 0));
	w.player(2)->radius(9);
	w.player(3)->position(Vector2(177, 235));
	w.player(3)->simulatedVelocity(Vector2(0, 0));
	w.player(3)->simulatedAcceleration(Vector2(0, 0));
	w.player(3)->radius(9);
	w.player(4)->position(Vector2(300, 235));
	w.player(4)->simulatedVelocity(Vector2(0, 0));
	w.player(4)->simulatedAcceleration(Vector2(0, 0));
	w.player(4)->radius(9);
	
	w.player(5)->position(Vector2(615, 235));
	w.player(5)->simulatedVelocity(Vector2(0, 0));
	w.player(5)->simulatedAcceleration(Vector2(0, 0));
	w.player(5)->radius(9);
	w.player(6)->position(Vector2(503, 130));
	w.player(6)->simulatedVelocity(Vector2(0, 0));
	w.player(6)->simulatedAcceleration(Vector2(0, 0));
	w.player(6)->radius(9);
	w.player(7)->position(Vector2(503, 340));
	w.player(7)->simulatedVelocity(Vector2(0, 0));
	w.player(7)->simulatedAcceleration(Vector2(0, 0));
	w.player(7)->radius(9);
	w.player(8)->position(Vector2(503, 235));
	w.player(8)->simulatedVelocity(Vector2(0, 0));
	w.player(8)->simulatedAcceleration(Vector2(0, 0));
	w.player(8)->radius(9);
	w.player(9)->position(Vector2(400, 235));
	w.player(9)->simulatedVelocity(Vector2(0, 0));
	w.player(9)->simulatedAcceleration(Vector2(0, 0));
	w.player(9)->radius(9);
	
	// Set the ball properties:
	Ball &ball = w.ball();
	ball.position(Vector2(330, 235));
	ball.simulatedVelocity(Vector2(0, 0));
	ball.simulatedAcceleration(Vector2(0, 0));
	ball.radius(2.15);
}

void Simulator::update() {	
	usleep(10000);

	World &world = World::get();
	const Field &field = world.field();

	for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
		PPlayer player = world.player(i);

		Vector2 pos = player->position();
		Vector2 vel = player->simulatedVelocity();
		Vector2 acc = player->simulatedAcceleration();
		
		//We may need this later but suppress warning for now
		//double maxacc = field.convertMmToCoord(MAX_ACCELERATION) / (CentralAnalyzingUnit::FRAMES_PER_SECOND * CentralAnalyzingUnit::FRAMES_PER_SECOND);
		double maxvel = field.convertMmToCoord(MAX_VELOCITY) / CentralAnalyzingUnit::FRAMES_PER_SECOND;
		/*if (acc.length() > maxacc){
			acc = acc / acc.length() * maxacc;
			player->simulatedAcceleration(acc);	
		}*/

		pos += vel/2.0;
		vel += acc;
		
		
		//if (vel.length() > maxvel)
		//	vel = vel / vel.length() * maxvel;
	
		//vel *= FRICTION_CONST;
		pos += vel/2.0;
		player->position(pos);
		player->simulatedVelocity(vel);
		
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

	Ball &ball = world.ball();
	Vector2 pos = ball.position();
	Vector2 vel = ball.simulatedVelocity();
	Vector2 acc = ball.simulatedAcceleration();

	if (vel.length() < 1E-9) acc = Vector2(0,0);
	else{
		acc = vel / (-vel.length());
		acc *= field.convertMmToCoord(BALL_ACCELERATION) / (CentralAnalyzingUnit::FRAMES_PER_SECOND * CentralAnalyzingUnit::FRAMES_PER_SECOND);
	}
	pos += vel/2.0;
	vel += acc;
	//pos += vel;
	
	//vel *= 0.98; // The ball has much less resistance than the robots.
	pos += vel/2.0;

	// Make sure the ball does not go out of bounds:
	if (pos.x < field.west()) {	
		if (pos.y > field.westGoal().north.y &&
			pos.y < field.westGoal().south.y) {
			if (world.friendlyTeam().side())
				world.enemyTeam().score(world.enemyTeam().score() + 1);
			else
				world.friendlyTeam().score(world.friendlyTeam().score() + 1);
		}
		world.playType(PlayType::start);
		pos = Vector2(field.centerCircle());
		vel = Vector2(0, 0);
		acc = Vector2(0, 0);
		for (unsigned int i = 0; i < 10; i++)
			if (world.player(i)->hasBall())
				world.player(i)->hasBall(false);
	} else if (pos.y < field.north()) {
		world.playType(PlayType::start);
		pos = Vector2(field.centerCircle());
		vel = Vector2(0, 0);
		acc = Vector2(0, 0);
		for (unsigned int i = 0; i < 10; i++)
			if (world.player(i)->hasBall())
				world.player(i)->hasBall(false);
	} else if (pos.x > field.east()) {
		if (pos.y > field.eastGoal().north.y &&
			pos.y < field.eastGoal().south.y) {
			if (world.friendlyTeam().side())
				world.friendlyTeam().score(world.friendlyTeam().score() + 1);
			else
				world.enemyTeam().score(world.enemyTeam().score() + 1);
		}
		world.playType(PlayType::start);
		pos = Vector2(field.centerCircle());
		vel = Vector2(0, 0);
		acc = Vector2(0, 0);
		for (unsigned int i = 0; i < 10; i++)
			if (world.player(i)->hasBall())
				world.player(i)->hasBall(false);
	} else if (pos.y > field.south()) {
		world.playType(PlayType::start);
		pos = Vector2(field.centerCircle());
		vel = Vector2(0, 0);
		acc = Vector2(0, 0);
		for (unsigned int i = 0; i < 10; i++)
			if (world.player(i)->hasBall())
				world.player(i)->hasBall(false);
	}

	switch(world.playType()){
		case PlayType::preparePenaltyKick:
			if (world.team(0).specialPossession())
				pos = Vector2(field.eastGoal().penalty);
			else
				pos = Vector2(field.westGoal().penalty);
			vel = Vector2(0,0);
			acc = Vector2(0,0);
			break;
	}
	
	ball.simulatedAcceleration(acc);
	ball.position(pos);
	ball.simulatedVelocity(vel);			
	
	// Keep track if there are multiple robots fighting over the ball
	// with one (first robot) and two (second robot).
	PPlayer one, two;
	for (unsigned int i = 0; i < 10; i++) {
		PPlayer cur = world.player(i);
		Vector2 diff = ball.position() - cur->position();
		if (diff.length() <= cur->radius() + ball.radius()) {
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
			ball.position(winner->position() + ballPos);
			ball.simulatedVelocity(winner->simulatedVelocity());
		}
	}

}

