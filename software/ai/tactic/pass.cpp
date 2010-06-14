#include "ai/tactic/pass.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pivot.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/move.h"
#include "ai/util.h"

#include <iostream>
#include <iterator>

namespace{

	class pass_state : public player::state {
		public:
			typedef Glib::RefPtr<pass_state> ptr;
	  pass_state(bool is_passer):passer(is_passer){
			}
	  bool passer;
	};

}



pass::pass(player::ptr player, world::ptr world, player::ptr receiver) : tactic(player), the_world(world), the_receiver(receiver) {
  //world
  pass_state::ptr state = pass_state::ptr(new pass_state(false));
  std::vector<player::ptr> team =  world->friendly.get_players();
  std::vector<player::ptr>::iterator team_iterator;
  for(team_iterator = team.begin(); team_iterator < team.end(); team_iterator++){
    (*team_iterator)->set_state(typeid(*this), state);
  }
  player->set_state(typeid(*this), pass_state::ptr(new pass_state(false)));
}

void pass::tick() {
	if (!ai_util::has_ball(the_player)) {
		pivot tactic(the_player, the_world);
		tactic.set_target(the_receiver->position());
		tactic.set_flags(flags);
		tactic.tick();
		return;
	}

	// bool should_pass = ai_util::can_pass(the_world, the_receiver);
	// do we need this velocity threshold?
	// && the_receiver->est_velocity().len() < ai_util::VEL_CLOSE;
	bool should_pass = true;

	if (should_pass) {
		std::cout << " pass: let's shoot " << std::endl;
		kick kick_tactic(the_player, the_world);
		kick_tactic.set_target(the_receiver->position());
		kick_tactic.set_flags(flags);
		kick_tactic.tick();
	} else {
		std::cout << " pass: move to receiver " << std::endl;
		move move_tactic(the_player, the_world);
		move_tactic.set_position(the_receiver->position());
		// ALSO FACE TOWARDS RECEIVER
		const point diff = the_receiver->position() - the_player->position();
		move_tactic.set_orientation(diff.orientation());
		move_tactic.set_flags(flags);
		move_tactic.tick();
	}
}

