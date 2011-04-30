#include "ai/hl/stp/action/move.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::move(const World &world, Player::Ptr player, const Point dest) {
	player->move(dest, (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
}

void AI::HL::STP::Action::move(Player::Ptr player, const double orientation, const Point dest) {
	player->move(dest, orientation, Point());
	player->type(AI::Flags::MoveType::NORMAL);
}

