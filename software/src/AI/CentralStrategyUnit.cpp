#include "AI/CentralStrategyUnit.h"
#include "AI/CentralAnalyzingUnit.h"
#include "AI/AITeam.h"
#include "datapool/Plan.h"
#include "datapool/PlayType.h"
#include "datapool/World.h"
#include "AI/Strategies/TestStrategy.h"
#include "AI/Strategies/OffensiveStrategy.h"
#include "AI/Strategies/LazyStrategy.h"
#include "AI/Strategies/DefenseStrategy.h"
#include "AI/Strategies/DibsStrategy.h"
#include "AI/Strategies/CheaterStrategy.h"

#include <cassert>

CentralStrategyUnit::CentralStrategyUnit(AITeam &team) : team(team) {
}

// temporary storage for ball position, used in special plays
Vector2 ballpos;

// distance to stay back from when a penalty kick is being taken
#define PENALTY_DISTANCE 1000.0 // normal value should be 400.0

void CentralStrategyUnit::update() {
	for (unsigned int id = 0; id < Team::SIZE; id++) {
		PPlayer robot = team.player(id);
		if (World::get().playType() == PlayType::play || World::get().playType() == PlayType::prepareKickoff)
			robot->allowedInside(true);
		else
			robot->allowedInside(false);
	}
	
	switch (World::get().playType()) {
		case PlayType::start:    		startingPositions(); break;
		case PlayType::stop:			stop(); break;
		case PlayType::play:     		strat->update(); break;
		case PlayType::directFreeKick:  directFreeKick(); break;
		case PlayType::indirectFreeKick:indirectFreeKick(); break;
		case PlayType::preparePenaltyKick:  preparePenaltyKick(); break;
		case PlayType::penaltyKick:		penaltyKick(); break;
		case PlayType::kickoff:    		kickoff(); break;
		case PlayType::penaltyKickoff:	penaltyKickoff(); break;
		case PlayType::victoryDance:    victoryDance(); break;
		case PlayType::doNothing:		doNothing(); break;
		case PlayType::prepareKickoff:	startingPositions(); break;//prepareKickoff(); break;
		default:                 		assert(false);
	}
}

PStrategy CentralStrategyUnit::strategy() const {
	return strat;
}

void CentralStrategyUnit::strategy(const PStrategy &s) {
	strat = s;
}

void CentralStrategyUnit::startingPositions() {
	World &w = World::get();
	PField field = w.field();
	
	int widthOffset = field->west();
	int fieldWidth = field->east() - field->west();
	int heightOffset = field->north();
	int fieldHeight = field->south() - field->north();

	static const double MULTIPLES[2][5][2] = {
		{{0.9754, 0.5}, {0.55, 0.35}, {0.55, 0.65}, {0.75, 0.5}, {0.62, 0.5}},
		{{0.0246, 0.5}, {0.45, 0.35}, {0.45, 0.65}, {0.25, 0.5}, {0.38, 0.5}}
	};
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		team.player(i)->plan(Plan::move);
		team.player(i)->destination(Vector2(widthOffset + fieldWidth * MULTIPLES[team.side()][i][0], heightOffset + fieldHeight * MULTIPLES[team.side()][i][1]));
		team.player(i)->hasBall(false);
	}
}

void CentralStrategyUnit::directFreeKick(){
	strat->update();
	if (team.specialPossession()) {
		
		World &w = World::get();
		PField field = w.field();
		
		// Find closest player to ball.
		double len = field->width() * 2.0;
		unsigned int closest = 0;
		for (unsigned int id = 1; id < Team::SIZE; id++) {
			Vector2 dis = w.ball().position() - team.player(id)->position();
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
		PPlayer kicker = team.player(closest);
		
		PPlayer passee = CentralAnalyzingUnit::closestRobot(kicker, CentralAnalyzingUnit::TEAM_SAME, false);

		for (unsigned int i = 0; i < Team::SIZE; i++) {
			PPlayer p = team.player(i);
			if (p->receivingPass())
				passee = p;
		}
		
		kicker->plan(Plan::passer);
		kicker->otherPlayer(passee);
		kicker->allowedInside(true);
		passee->receivingPass(true);
	}
}

void CentralStrategyUnit::preparePenaltyKick(){
	strat->update();
		
	World &w = World::get();
	PField field = w.field();
	ballpos = w.ball().position();
	if (team.specialPossession()) {
		
		// Find closest player to ball.
		double len = field->width() * 2.0;
		unsigned int closest = 0;
		for (unsigned int id = 1; id < Team::SIZE; id++) {
			Vector2 dis = w.ball().position() - team.player(id)->position();
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
		PPlayer kicker = team.player(closest);
		kicker->plan(Plan::chase);
		kicker->allowedInside(true);

		for (unsigned int id = 1; id < Team::SIZE; id++)
			if (id != closest){
				if (ballpos.x >= field->centerCircle().x){
					double maxx = w.ball().position().x - 2 * team.player(id)->radius() - field->convertMmToCoord(PENALTY_DISTANCE);
					//if(team.player(id)->destination().x > maxx)
						team.player(id)->destination(Vector2(maxx, team.player(id)->destination().y));
				}
				else{
					double minx = w.ball().position().x + 2 * team.player(id)->radius() + field->convertMmToCoord(PENALTY_DISTANCE);
					//if(team.player(id)->destination().x < minx)
						team.player(id)->destination(Vector2(minx, team.player(id)->destination().y));
				}
				team.player(id)->plan(Plan::move);
				team.player(id)->receivingPass(false);
			}
	}   
        else {
		for (unsigned int id = 1; id < Team::SIZE; id++){
			if (ballpos.x < field->centerCircle().x){
				double minx = w.ball().position().x + 2*team.player(id)->radius() + field->convertMmToCoord(PENALTY_DISTANCE);
				//if(team.player(id)->destination().x < minx)
					team.player(id)->destination(Vector2(minx, team.player(id)->destination().y));
			}
			else{
				double maxx = w.ball().position().x - 2*team.player(id)->radius() - field->convertMmToCoord(PENALTY_DISTANCE);
				//if(team.player(id)->destination().x > maxx)
					team.player(id)->destination(Vector2(maxx, team.player(id)->destination().y));
			}
			team.player(id)->plan(Plan::move);
			team.player(id)->receivingPass(false);
		}
		PPlayer goalie = team.player(0);
		goalie->allowedInside(true);
		if (ballpos.x < field->centerCircle().x){
			double maxx = field->west() + goalie->radius() / 2;
			if(goalie->destination().x > maxx)
				goalie->destination(Vector2(maxx, goalie->destination().y));
		}
		else{
			double minx = field->east() - goalie->radius() / 2;
			if(goalie->destination().x < minx)
				goalie->destination(Vector2(minx, goalie->destination().y));
		}
		goalie->plan(Plan::goalie);
        }
              
}

void CentralStrategyUnit::penaltyKick(){

strat->update();
		
	World &w = World::get();
	PField field = w.field();
	if ((w.ball().position()-ballpos).length() > field->convertMmToCoord(10.0)){
		w.playType(PlayType::play);
		return;	
	}
	if (team.specialPossession()) {
		
		// Find closest player to ball.
		double len = field->width() * 2.0;
		unsigned int closest = 0;
		for (unsigned int id = 1; id < Team::SIZE; id++) {
			Vector2 dis = w.ball().position() - team.player(id)->position();
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
		PPlayer kicker = team.player(closest);
		kicker->plan(Plan::shoot);
		kicker->allowedInside(true);

                for (unsigned int id = 1; id < Team::SIZE; id++)
			if (id != closest){
				if (ballpos.x >= field->centerCircle().x){
					double maxx = w.ball().position().x - 2*team.player(id)->radius() - field->convertMmToCoord(PENALTY_DISTANCE);
					//if(team.player(id)->destination().x > maxx)
						team.player(id)->destination(Vector2(maxx, team.player(id)->destination().y));
				}
				else{
					double minx = w.ball().position().x + 2*team.player(id)->radius() + field->convertMmToCoord(PENALTY_DISTANCE);
					//if(team.player(id)->destination().x < minx)
						team.player(id)->destination(Vector2(minx, team.player(id)->destination().y));
				}
				team.player(id)->plan(Plan::move);
				team.player(id)->receivingPass(false);
			}
	}   
        else {
		for (unsigned int id = 1; id < Team::SIZE; id++){
			if (ballpos.x < field->centerCircle().x){
				double minx = w.ball().position().x + 2*team.player(id)->radius() + field->convertMmToCoord(PENALTY_DISTANCE);
				//if(team.player(id)->destination().x < minx)
					team.player(id)->destination(Vector2(minx, team.player(id)->destination().y));
			}
			else{
				double maxx = w.ball().position().x - 2*team.player(id)->radius() - field->convertMmToCoord(PENALTY_DISTANCE);
				//if(team.player(id)->destination().x > maxx)
					team.player(id)->destination(Vector2(maxx, team.player(id)->destination().y));
			}
			team.player(id)->plan(Plan::move);
			team.player(id)->receivingPass(false);
		}
		PPlayer goalie = team.player(0);
		goalie->allowedInside(true);
		if (ballpos.x < field->centerCircle().x){
			double maxx = field->west() + goalie->radius() / 2;
			if(goalie->destination().x > maxx)
				goalie->destination(Vector2(maxx, goalie->destination().y));
		}
		else{
			double minx = field->east() - goalie->radius() / 2;
			if(goalie->destination().x < minx)
				goalie->destination(Vector2(minx, goalie->destination().y));
		}
		goalie->plan(Plan::goalie);
        }
}

void CentralStrategyUnit::prepareKickoff(){
	strat->update();
	if (team.specialPossession()) {
		
		World &w = World::get();
		PField field = w.field();
		
		// Find closest player to ball.
		double len = field->width() * 2.0;
		unsigned int closest = 0;
		for (unsigned int id = 1; id < Team::SIZE; id++) {
			Vector2 dis = w.ball().position() - team.player(id)->position();
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
		PPlayer kicker = team.player(closest);
		
		kicker->plan(Plan::chase);
	}
	
}

void CentralStrategyUnit::kickoff() {
	strat->update();
	World &w = World::get();
	PField field = w.field();
	if ((w.ball().position()-field->centerCircle()).length() > field->convertMmToCoord(10.0)){
		w.playType(PlayType::play);
		return;	
	}
	if (team.specialPossession()) {
		
		// Find closest player to ball.
		double len = field->width() * 2.0;
		unsigned int closest = 0;
		for (unsigned int id = 1; id < Team::SIZE; id++) {
			Vector2 dis = w.ball().position() - team.player(id)->position();
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
		PPlayer kicker = team.player(closest);
		
		PPlayer passee = CentralAnalyzingUnit::closestRobot(kicker, CentralAnalyzingUnit::TEAM_SAME, false);

		for (unsigned int i = 0; i < Team::SIZE; i++) {
			PPlayer p = team.player(i);
			if (p->receivingPass())
				passee = p;
		}
		
		kicker->plan(Plan::passer);
		kicker->allowedInside(true);
		kicker->otherPlayer(passee);
		passee->receivingPass(true);
	}
}

void CentralStrategyUnit::stop(){
	strat->update();
	for (unsigned int id = 1; id < Team::SIZE; id++){
		team.player(id)->hasBall(false);

	}
	if (team.specialPossession()) {
		
		World &w = World::get();
		PField field = w.field();
		
		// Find closest player to ball.
		double len = field->width() * 2.0;
		unsigned int closest = 0;
		for (unsigned int id = 1; id < Team::SIZE; id++) {
			Vector2 dis = w.ball().position() - team.player(id)->position();
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
		PPlayer kicker = team.player(closest);
		
		kicker->plan(Plan::chase);
		
		ballpos = w.ball().position();
	}
	
}

void CentralStrategyUnit::indirectFreeKick() {
	strat->update();
	World &w = World::get();
	PField field = w.field();
	if ((w.ball().position()-ballpos).length() > field->convertMmToCoord(10.0)){
		w.playType(PlayType::play);
		return;	
	}
	if (team.specialPossession()) {
		
		// Find closest player to ball.
		double len = field->width() * 2.0;
		unsigned int closest = 0;
		for (unsigned int id = 1; id < Team::SIZE; id++) {
			Vector2 dis = w.ball().position() - team.player(id)->position();
			if (dis.length() < len) {
				closest = id;
				len = dis.length();
			}
		}
		PPlayer kicker = team.player(closest);
		
		PPlayer passee = CentralAnalyzingUnit::closestRobot(kicker, CentralAnalyzingUnit::TEAM_SAME, false);

		for (unsigned int i = 0; i < Team::SIZE; i++) {
			PPlayer p = team.player(i);
			if (p->receivingPass())
				passee = p;
		}
		
		kicker->plan(Plan::passer);
		kicker->allowedInside(true);
		kicker->otherPlayer(passee);
		passee->receivingPass(true);
	}
}

void CentralStrategyUnit::penaltyKickoff() {

}

void CentralStrategyUnit::victoryDance() {

}

void CentralStrategyUnit::doNothing() {
	for (unsigned int id = 0; id < Team::SIZE; id++) {
		PPlayer robot = team.player(id);
		robot->plan(Plan::stop);
	}
}
