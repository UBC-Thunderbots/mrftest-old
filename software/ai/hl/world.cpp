#include "ai/hl/world.h"

using namespace AI::HL::W;

void AI::HL::W::Player::move(Point dest, double ori, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) {
	move(dest, ori, Point(), flags, type, prio);
}

World::World() {
}

World::~World() {
}

