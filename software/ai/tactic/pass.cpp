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

	class pass_state : public player::state {
		public:
			typedef Glib::RefPtr<pass_state> ptr;
			pass_state(bool is_passer):passer(is_passer){
			}
			bool passer;
	};

}
*/

pass::pass(player::ptr player, world::ptr world, player::ptr receiver) : tactic(player), the_world(world), the_receiver(receiver) {
	//world
	/*
	pass_state::ptr state = pass_state::ptr(new pass_state(false));
	std::vector<player::ptr> team =  world->friendly.get_players();
	std::vector<player::ptr>::iterator team_iterator;
	for(team_iterator = team.begin(); team_iterator < team.end(); team_iterator++){
		(*team_iterator)->set_state(typeid(*this), state);
	}
	player->set_state(typeid(*this), pass_state::ptr(new pass_state(false)));
	*/
}

void pass::tick() {
	if (!ai_util::has_ball(the_world, the_player)) {
		pivot tactic(the_player, the_world);
		tactic.set_target(the_receiver->position());
		tactic.set_flags(flags);
		tactic.tick();
		return;
	}

	
	if (the_player->sense_ball_time() > ai_util::DRIBBLE_TIMEOUT) {
		shoot tactic(the_player, the_world);
		tactic.force();
		tactic.set_flags(flags);
		tactic.tick();
		return;
	}
	

	const point diff = the_receiver->position() - the_player->position();
	const double anglediff = angle_diff(diff.orientation(), the_player->orientation());
	bool should_pass = (anglediff < ai_util::ORI_CLOSE) && (ai_util::can_receive(the_world, the_receiver) /*|| the_player->sense_ball_time() > ai_util::DRIBBLE_TIMEOUT*/);

	// do we need this velocity threshold?
	// && the_receiver->est_velocity().len() < ai_util::VEL_CLOSE;

	move move_tactic(the_player, the_world);

	if (the_player->dribble_distance() < player::MAX_DRIBBLE_DIST) {
		move_tactic.set_position(the_receiver->position());
	}

	move_tactic.set_orientation(diff.orientation());

	if (should_pass) {
		if (the_player->chicker_ready_time() == 0) {
			LOG_INFO(Glib::ustring::compose("%1 kick", the_player->name));
			the_player->kick(1.0);
		} else {
			LOG_INFO(Glib::ustring::compose("%1 chicker not ready", the_player->name));
		}
	}

	move_tactic.set_flags(flags);
	move_tactic.tick();

}

