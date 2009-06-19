#include "datapool/Player.h"
#include "datapool/World.h"

PPlayer Player::create(Team &team, unsigned int id) {
	PPlayer player(new Player(team, id));
	return player;
}

Player::Player(Team &team, unsigned int id) : idx(id), orient(0), possession(false), receiving(false), tm(team) {
}

Plan::Behavior Player::plan() const {
	return behavior;
}

void Player::plan(Plan::Behavior plan) {
	behavior = plan;
}

double Player::orientation() const {
	return orient;
}

void Player::orientation(double o) {
	while (o >= 360)
		o -= 360;
	while (o < 0)
		o += 360;
	orient = o;
}

bool Player::hasBall() const {
	return possession;
}

void Player::hasBall(bool hasBall) {
	possession = hasBall;
}

bool Player::receivingPass() const {
	return receiving;
}

void Player::receivingPass(bool pass) {
	receiving = pass;

	// Make sure all the other players on the team are no longer receiving passes.
	for (unsigned int i = 0; i < Team::SIZE; i++)
		if (tm.player(i)->id() != id() && tm.player(i)->receivingPass())
			tm.player(i)->receivingPass(false);
}

Team &Player::team() {
	return tm;
}

const Team &Player::team() const {
	return tm;
}

const PPlayer Player::otherPlayer() const {
	return other;
}

PPlayer Player::otherPlayer() {
	return other;
}

void Player::otherPlayer(const PPlayer &o) {
	other = o;
}

const Vector2 &Player::destination() const {
	return dest;
}

void Player::destination(const Vector2 &d) {
	dest = d;
}

bool Player::allowedInside() const {
	return allowedIn;
}

void Player::allowedInside(bool val) {
	allowedIn = val;
}

const Vector2 &Player::requestedVelocity() const {
	return reqVelocity;
}

void Player::requestedVelocity(const Vector2 &rv) {
	reqVelocity = rv;
}

unsigned int Player::id() const {
	return idx;
}

