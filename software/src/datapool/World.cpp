#include "datapool/World.h"
#include "datapool/Team.h"
#include "AI/AITeam.h"

#include <cassert>



namespace {
	World *instance = 0;
}

void World::init(Team &friendlyTeam, Team &enemyTeam, const Field &field) {
	assert(!instance);
	instance = new World(friendlyTeam, enemyTeam, field);
}

World &World::get() {
	assert(instance);
	return *instance;
}

World::World(Team &friendlyTeam, Team &enemyTeam, const Field &field) : play(PlayType::play), friendly(friendlyTeam), enemy(enemyTeam), field_(field), ballVisible(false) {
	for (unsigned int j = 0; j < Team::SIZE; j++)
		everyone.push_back(friendly.player(j));
	for (unsigned int j = 0; j < Team::SIZE; j++)
		everyone.push_back(enemy.player(j));
}

Team &World::friendlyTeam() {
	return friendly;
}

const Team &World::friendlyTeam() const {
	return friendly;
}

Team &World::enemyTeam() {
	return enemy;
}

const Team &World::enemyTeam() const {
	return enemy;
}

Team &World::team(unsigned int id) {
	assert(id < 2);
	return id == 0 ? friendly : enemy;
}

const Team &World::team(unsigned int id) const {
	assert(id < 2);
	return id == 0 ? friendly : enemy;
}

Field &World::field() {
	return field_;
}

const Field &World::field() const {
	return field_;
}

void World::update() {
	friendly.update();
	enemy.update();
}

PlayType::Type World::playType() const {
	return play;
}

void World::playType(PlayType::Type type) {
	play = type;
}

PPlayer World::player(unsigned int idx) {
	if (idx >= Team::SIZE)
		return enemy.player(idx - Team::SIZE);
	else
		return friendly.player(idx);
}

const PPlayer World::player(unsigned int idx) const {
	if (idx >= Team::SIZE)
		return enemy.player(idx - Team::SIZE);
	else
		return friendly.player(idx);
}

const std::vector<PPlayer> &World::players() {
	return everyone;
}

Ball &World::ball() {
	return ball_;
}

const Ball &World::ball() const {
	return ball_;
}

bool World::isBallVisible() const {
	return ballVisible;
}

void World::isBallVisible(bool newVal) {
	ballVisible = newVal;
}

