#include "AI/AITeam.h"

AITeam::AITeam(unsigned int id) : Team(id), du(*this), csu(*this), lsu(*this) {
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

