#include "ai/hl/stp/action/ram.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::ram(const World &world, Player::Ptr player, const Point dest, const Point vel) {
	player->move(dest, (world.ball().position() - player->position()).orientation(), vel);
	player->type(AI::Flags::MoveType::RAM_BALL);
	player->prio(AI::Flags::MovePrio::HIGH);
}
