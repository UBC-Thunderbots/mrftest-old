#include "ai/tactic/patrol.h"
#include "ai/util.h"

patrol::patrol(player::ptr player, world::ptr world, const unsigned int& flags) : the_player(player), 
navi(player, world), 
should_move_to_first(true),
move_tactic1(new move(player, world, flags)),
move_tactic2(new move(player, world, flags)) {
}

patrol::patrol(player::ptr player, world::ptr world, const unsigned int& flags, const point& position1, const point& position2) : tactic(flags), 
the_player(player), 
navi(player, world), 
should_move_to_first(true),
move_tactic1(new move(player, world, flags)),
move_tactic2(new move(player, world, flags)) {
	move_tactic1->set_position(position1);
	move_tactic2->set_position(position2);
	the_position1 = position1;
	the_position2 = position2;
}

void patrol::set_targets(const point& position1, const point& position2)
{
	move_tactic1->set_position(position1);
	move_tactic2->set_position(position2);
	the_position1 = position1;
	the_position2 = position2;
}

void patrol::tick() {
	if (!move_tactic1->is_position_set() || !move_tactic2->is_position_set())
		return;

	if (should_move_to_first) {
		// reached the first position
		if ((the_player->position() - the_position1).lensq() <= ai_util::POS_CLOSE) {
			should_move_to_first = false;
			move_tactic2->tick();
		} else {
			move_tactic1->tick();
		}
	} else {
		// reached the end position
		if ((the_player->position() - the_position2).lensq() <= ai_util::POS_CLOSE) {
			should_move_to_first = true;
			move_tactic1->tick();
		} else {
			move_tactic2->tick();
		}
	}

}

