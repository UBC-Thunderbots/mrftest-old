#include "datapool/World.h"
#include "datapool/Team.h"
#include "AI/AITeam.h"

#include <cassert>



namespace {
	World *instance = 0;
}

void World::init(PTeam friendlyTeam, PTeam enemyTeam, PField field) {
	assert(!instance);
	instance = new World(friendlyTeam, enemyTeam, field);
}

World &World::get() {
	assert(instance);
	return *instance;
}

World::World(PTeam friendlyTeam, PTeam enemyTeam, PField field) : play(PlayType::play), field_(field), ball_(Ball::create()) {
	teams[0] = friendlyTeam;
	teams[1] = enemyTeam;
	for (unsigned int i = 0; i < 2; i++)
		for (unsigned int j = 0; j < Team::SIZE; j++)
			everyone.push_back(teams[i]->player(j));
}

PTeam World::friendlyTeam() {
	return teams[0];
}

const PTeam World::friendlyTeam() const {
	return teams[0];
}

PTeam World::enemyTeam() {
	return teams[1];
}

const PTeam World::enemyTeam() const {
	return teams[1];
}

PTeam World::team(unsigned int id) {
	assert(id < 2);
	return teams[id];
}

const PTeam World::team(unsigned int id) const {
	assert(id < 2);
	return teams[id];
}

PField World::field() {
	return field_;
}

const PField World::field() const {
	return field_;
}

void World::update() {
	for (unsigned int i = 0; i < sizeof(teams)/sizeof(*teams); i++)
		teams[i]->update();
}

PlayType::Type World::playType() const {
	return play;
}

void World::playType(PlayType::Type type) {
	play = type;
}

PPlayer World::player(unsigned int idx) {
	return teams[idx / Team::SIZE]->player(idx % Team::SIZE);
}

const PPlayer World::player(unsigned int idx) const {
	return teams[idx / Team::SIZE]->player(idx % Team::SIZE);
}

const std::vector<PPlayer> &World::players() {
	return everyone;
}

PBall World::ball() {
	return ball_;
}

const PBall World::ball() const {
	return ball_;
}

