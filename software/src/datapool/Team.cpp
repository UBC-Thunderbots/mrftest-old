#include "datapool/Team.h"
#include "datapool/World.h"

#include <cassert>

Team::Team(unsigned int id) : robots(SIZE), west(false), special(false), points(0), id(id), activePl(SIZE) {
	for (unsigned int i = 0; i < SIZE; i++)
		robots[i] = Player::create(*this, id * Team::SIZE + i);
}

void Team::update() {
}

PPlayer Team::player(unsigned int index) {
	assert(index < SIZE);
	return robots[index];
}

const PPlayer Team::player(unsigned int index) const {
	assert(index < SIZE);
	return robots[index];
}

const std::vector<PPlayer> &Team::players() {
	return robots;
}

bool Team::side() const {
	return west;
}

void Team::side(bool w) {
	west = w;
}

bool Team::specialPossession() const {
	return special;
}

void Team::specialPossession(bool sp) {
	special = sp;
}

unsigned int Team::score() const {
	return points;
}

void Team::score(unsigned int pts) {
	points = pts;
}

Team &Team::other() {
	return World::get().team(!id);
}

const Team &Team::other() const {
	return World::get().team(!id);
}

unsigned int Team::activePlayers() const {
	return activePl;
}

void Team::activePlayers(unsigned int n) {
	assert(n <= SIZE);
	activePl = n;
}

unsigned int Team::index() const {
	return id;
}

