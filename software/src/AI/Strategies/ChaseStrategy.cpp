#include "AI/Strategies/ChaseStrategy.h"
#include "AI/AITeam.h"
#include "datapool/World.h"
#include "AI/RobotController.h"

#include <iostream>
using namespace std;

ChaseStrategy::ChaseStrategy(AITeam &team) : Strategy(team) {	
}

void ChaseStrategy::update() {
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		Vector2 orientation = World::get().ball()->position() - team.player(i)->position();
		/*
		Vector2 des = 10*(World::get().ball()->position()-team.player(i)->position())+team.player(i)->position();
		cout << " player=" << team.player(i)->position().x << " " << team.player(i)->position().y << endl;
		cout << " ball=" << World::get().ball()->position().x << " " << World::get().ball()->position().y << endl;
		cout << " orientation=" << orientation.x << " " << orientation.y << endl;
		cout << " des=" << des.x << " " << des.y << endl;
		*/
		team.player(i)->plan(Plan::move);
		Vector2 des = Vector2(orientation.angle()+90)*orientation.length();
		//team.player(i)->destination(orientation + team.player(i)->position());
		team.player(i)->destination(des + team.player(i)->position());
	}
		
	//team.player(0)->plan(Plan::goalie);
	
	/*
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		if (team.player(i)->hasBall()) {
			//team.player(i)->plan(Plan::stop);
			Vector2 orientation = World::get().ball()->position() - team.player(i)->position();
			double angle = orientation.angle();
			RobotController::sendCommand(team.player(i), Vector2(orientation.angle()), angle-90, 255, 0);
		}
	}*/
}

