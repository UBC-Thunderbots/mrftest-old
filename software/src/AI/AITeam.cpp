#include "AI/AITeam.h"

PAITeam AITeam::create(unsigned int id) {
	PAITeam team(new AITeam(id));
	for (unsigned int i = 0; i < SIZE; i++)
		team->robots[i] = Player::create(team);
	return team;
}

AITeam::AITeam(unsigned int id) : Team(id), du(*this), csu(*this), lsu(*this) {
}

AITeam::~AITeam() {
}

void AITeam::update() {
	du.update();
	csu.update();
	lsu.update();
}

DecisionUnit &AITeam::getDU() {
	return du;
}

const DecisionUnit &AITeam::getDU() const {
	return du;
}

CentralStrategyUnit &AITeam::getCSU() {
	return csu;
}

const CentralStrategyUnit &AITeam::getCSU() const {
	return csu;
}

LocalStrategyUnit &AITeam::getLSU() {
	return lsu;
}

const LocalStrategyUnit &AITeam::getLSU() const {
	return lsu;
}

