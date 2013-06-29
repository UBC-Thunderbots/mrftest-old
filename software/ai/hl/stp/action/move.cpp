#include "ai/hl/stp/action/move.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::move(World world, Player player, const Point dest) {
	player.move(dest, (world.ball().position() - player.position()).orientation(), Point());
	player.type(AI::Flags::MoveType::NORMAL);
}

void AI::HL::STP::Action::move(World world, Player player, const Point dest, const Point vel) {
	player.move(dest, (world.ball().position() - player.position()).orientation(), vel);
	player.type(AI::Flags::MoveType::NORMAL);
}

void AI::HL::STP::Action::move(Player player, const Angle orientation, const Point dest) {
	player.move(dest, orientation, Point());
	player.type(AI::Flags::MoveType::NORMAL);
}

void AI::HL::STP::Action::move(Player player, const Angle orientation, const Point dest, const Point vel) {
	player.move(dest, orientation, vel);
	player.type(AI::Flags::MoveType::NORMAL);
}

void AI::HL::STP::Action::move_careful(World world, Player player, const Point dest) {
	player.move(dest, (world.ball().position() - player.position()).orientation(), Point());
	player.type(AI::Flags::MoveType::MOVE_CAREFUL);
}

