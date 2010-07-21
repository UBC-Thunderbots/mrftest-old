#include "ai/tactic/pass.h"
#include "ai/tactic/pivot.h"
#include "ai/tactic/move.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "ai/tactic/shoot.h"

#include <iostream>
#include <iterator>

/*
namespace {

	class PassState : public Player::State {
		public:
			typedef RefPtr<PassState> ptr;
			PassState(bool is_passer):passer(is_passer){
			}
			bool passer;
	};

}
*/

Pass::Pass(Player::ptr player, World::ptr world, Player::ptr receiver) : Tactic(player), the_world(world), the_receiver(receiver) {
	//world
	/*
	PassState::ptr state = PassState::ptr(new PassState(false));
	std::vector<Player::ptr> team =  world->friendly.get_players();
	std::vector<Player::ptr>::iterator team_iterator;
	for(team_iterator = team.begin(); team_iterator < team.end(); team_iterator++){
		(*team_iterator)->set_state(typeid(*this), state);
	}
	player->set_state(typeid(*this), PassState::ptr(new PassState(false)));
	*/
}

void Pass::tick() {
	if (!AIUtil::has_ball(the_world, player)) {
		Pivot tactic(player, the_world);
		tactic.set_target(the_receiver->position());
		tactic.set_flags(flags);
		tactic.tick();
		return;
	}

	
	if (player->sense_ball_time() > AIUtil::DRIBBLE_TIMEOUT) {
		Shoot tactic(player, the_world);
		tactic.force();
		tactic.set_flags(flags);
		tactic.tick();
		return;
	}
	

	const Point diff = the_receiver->position() - player->position();
	const double anglediff = angle_diff(diff.orientation(), player->orientation());
	bool should_pass = (anglediff < AIUtil::ORI_CLOSE) && (AIUtil::can_receive(the_world, the_receiver) /*|| player->sense_ball_time() > AIUtil::DRIBBLE_TIMEOUT*/);

	// do we need this velocity threshold?
	// && the_receiver->est_velocity().len() < AIUtil::VEL_CLOSE;

	Move move_tactic(player, the_world);

	if (player->dribble_distance() < Player::MAX_DRIBBLE_DIST) {
		move_tactic.set_position(the_receiver->position());
	}

	move_tactic.set_orientation(diff.orientation());

	if (should_pass) {
		if (player->chicker_ready_time() == 0) {
			LOG_INFO(Glib::ustring::compose("%1 kick", player->name));
			player->kick(1.0);
		} else {
			LOG_INFO(Glib::ustring::compose("%1 chicker not ready", player->name));
		}
	}

	move_tactic.set_flags(flags);
	move_tactic.tick();

}

